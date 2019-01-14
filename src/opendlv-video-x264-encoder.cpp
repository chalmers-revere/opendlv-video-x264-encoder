/*
 * Copyright (C) 2018  Christian Berger
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

extern "C" {
    #include <x264.h>
}
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

int32_t main(int32_t argc, char **argv) {
    int32_t retCode{1};
    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid")) ||
         (0 == commandlineArguments.count("name")) ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ) {
        std::cerr << argv[0] << " attaches to an I420-formatted image residing in a shared memory area to convert it into a corresponding h264 frame for publishing to a running OD4 session." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDaVINCI session> --name=<name of shared memory area> --width=<width> --height=<height> [--gop=<GOP>] [--preset=X] [--verbose] [--id=<identifier in case of multiple instances]" << std::endl;
        std::cerr << "         --cid:      CID of the OD4Session to send h264 frames" << std::endl;
        std::cerr << "         --id:       when using several instances, this identifier is used as senderStamp" << std::endl;
        std::cerr << "         --name:     name of the shared memory area to attach" << std::endl;
        std::cerr << "         --width:    width of the frame" << std::endl;
        std::cerr << "         --height:   height of the frame" << std::endl;
        std::cerr << "         --gop:      optional: length of group of pictures (default = 10)" << std::endl;
        std::cerr << "         --preset:   one of x264's presets: ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow; default: veryfast" << std::endl;
        std::cerr << "         --verbose:  print encoding information" << std::endl;
        std::cerr << "Example: " << argv[0] << " --cid=111 --name=data --width=640 --height=480 --verbose" << std::endl;
    }
    else {
        const std::string NAME{commandlineArguments["name"]};
        const uint32_t WIDTH{static_cast<uint32_t>(std::stoi(commandlineArguments["width"]))};
        const uint32_t HEIGHT{static_cast<uint32_t>(std::stoi(commandlineArguments["height"]))};
        const uint32_t GOP_DEFAULT{10};
        const uint32_t GOP{(commandlineArguments["gop"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["gop"])) : GOP_DEFAULT};
        const std::string PRESET{(commandlineArguments["preset"].size() != 0) ? commandlineArguments["preset"] : "veryfast"};
        const bool VERBOSE{commandlineArguments.count("verbose") != 0};
        const uint32_t ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};

        std::unique_ptr<cluon::SharedMemory> sharedMemory(new cluon::SharedMemory{NAME});
        if (sharedMemory && sharedMemory->valid()) {
            std::clog << "[opendlv-video-x264-encoder]: Attached to '" << sharedMemory->name() << "' (" << sharedMemory->size() << " bytes)." << std::endl;

            // Configure x264 parameters.
            x264_param_t parameters;
            if (0 != x264_param_default_preset(&parameters, PRESET.c_str(), "zerolatency")) {
                std::cerr << "[opendlv-video-x264-encoder]: Failed to load preset parameters (" << PRESET << ", zerolatency) for x264." << std::endl;
                return 1;
            }
            parameters.i_width  = WIDTH;
            parameters.i_height = HEIGHT;
            parameters.i_log_level = (VERBOSE ? X264_LOG_INFO : X264_LOG_NONE);
            parameters.i_csp = X264_CSP_I420;
            parameters.i_bitdepth = 8;
            parameters.i_threads = 1;
            parameters.i_keyint_min = GOP;
            parameters.i_keyint_max = GOP;
            parameters.i_fps_num = 20 /* implicitly derived from SharedMemory notifications */;
            parameters.b_vfr_input = 0;
            parameters.b_repeat_headers = 1;
            parameters.b_annexb = 1;
            if (0 != x264_param_apply_profile(&parameters, "baseline")) {
                std::cerr << "[opendlv-video-x264-encoder]:Failed to apply parameters for x264." << std::endl;
                return 1;
            }

            // Initialize picture to pass YUV420 data into encoder.
            x264_picture_t picture_in;
            x264_picture_init(&picture_in);
            picture_in.i_type = X264_TYPE_AUTO;
            picture_in.img.i_csp = X264_CSP_I420;
            picture_in.img.i_plane = HEIGHT;

            // Directly point to the shared memory.
            sharedMemory->lock();
            {
                picture_in.img.plane[0] = reinterpret_cast<uint8_t*>(sharedMemory->data());
                picture_in.img.plane[1] = reinterpret_cast<uint8_t*>(sharedMemory->data() + (WIDTH * HEIGHT));
                picture_in.img.plane[2] = reinterpret_cast<uint8_t*>(sharedMemory->data() + (WIDTH * HEIGHT + ((WIDTH * HEIGHT) >> 2)));
                picture_in.img.i_stride[0] = WIDTH;
                picture_in.img.i_stride[1] = WIDTH/2;
                picture_in.img.i_stride[2] = WIDTH/2;
                picture_in.img.i_stride[3] = 0;
            }
            sharedMemory->unlock();

            // Open h264 encoder.
            x264_t *encoder = x264_encoder_open(&parameters);
            if (nullptr == encoder) {
                std::cerr << "[opendlv-video-x264-encoder]: Failed to open x264 encoder." << std::endl;
                return 1;
            }

            cluon::data::TimeStamp before, after, sampleTimeStamp;

            // Interface to a running OpenDaVINCI session (ignoring any incoming Envelopes).
            cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"]))};

            int i_frame{0};
            while ( (sharedMemory && sharedMemory->valid()) && od4.isRunning() ) {
                // Wait for incoming frame.
                sharedMemory->wait();

                sampleTimeStamp = cluon::time::now();

                std::string data;
                sharedMemory->lock();
                {
                    // Read notification timestamp.
                    auto r = sharedMemory->getTimeStamp();
                    sampleTimeStamp = (r.first ? r.second : sampleTimeStamp);
                }
                {
                    if (VERBOSE) {
                        before = cluon::time::now();
                    }
                    x264_nal_t *nals{nullptr};
                    int i_nals{0};
                    picture_in.i_pts = i_frame++;
                    x264_picture_t picture_out;
                    int frameSize{x264_encoder_encode(encoder, &nals, &i_nals, &picture_in, &picture_out)};
                    if (0 < frameSize) {
                        data = std::string(reinterpret_cast<char*>(nals->p_payload), frameSize);
                    }
                    if (VERBOSE) {
                        after = cluon::time::now();
                    }
                }
                sharedMemory->unlock();

                if (!data.empty()) {
                    opendlv::proxy::ImageReading ir;
                    ir.fourcc("h264").width(WIDTH).height(HEIGHT).data(data);
                    od4.send(ir, sampleTimeStamp, ID);

                    if (VERBOSE) {
                        std::clog << "[opendlv-video-x264-encoder]: Frame size = " << data.size() << " bytes; sample time = " << cluon::time::toMicroseconds(sampleTimeStamp) << " microseconds; encoding took " << cluon::time::deltaInMicroseconds(after, before) << " microseconds." << std::endl;
                    }
                }
            }

            x264_encoder_close(encoder);
            retCode = 0;
        }
        else {
            std::cerr << "[opendlv-video-x264-encoder]: Failed to attach to shared memory '" << NAME << "'." << std::endl;
        }
    }
    return retCode;
}
