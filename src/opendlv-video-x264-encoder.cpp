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
    const uint32_t ZERO{0};
    const uint32_t ONE{1};
    const uint32_t TWO{2};
    const uint32_t FOUR{4};
    const uint32_t TEN{10};
    const uint32_t SIXTEEN{16};
    const uint32_t FIFTYONE{51};
    const uint32_t HUNDRED{100};
    const uint32_t THOUSAND{1000};

    auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
    if ( (0 == commandlineArguments.count("cid")) ||
         (0 == commandlineArguments.count("name")) ||
         (0 == commandlineArguments.count("width")) ||
         (0 == commandlineArguments.count("height")) ) {
        std::cerr << argv[0] << " attaches to an I420-formatted image residing in a shared memory area to convert it into a corresponding h264 frame for publishing to a running OD4 session." << std::endl;
        std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDaVINCI session> --name=<name of shared memory area> --width=<width> --height=<height> [--gop=<GOP>] [--preset=X] [--verbose] [--id=<identifier in case of multiple instances]"
                "[--scenecut=<scenecut>] [--intra-refresh=<intra-refresh>] [--bframe=<bframe>] [--badapt=<badapt>] [--cabac=<cabac>] [--threads=<threads>] [--rc-mode=<rc-mode>] [--tune=Y] [--qp=<qp>] [--qpmin=<qpmin>]"
                "[--qpmax=<qpmax>] [--qpstep=<qpstep>] [--bitrate=<bitrate>] [--crf=<crf>] [--ipratio=<ipratio>] [--pbratio=<pbratio>] [--aq-mode=<aq-mode>] [--aq-strength=<aq-strength>] [--weightp=<weightp>]"
                "[--me=<me>] [--merange=<merange>] [--subme=<subme>] [--trellis=<trellis>]" << std::endl;
        std::cerr << "         --cid:           CID of the OD4Session to send h264 frames" << std::endl;
        std::cerr << "         --id:            when using several instances, this identifier is used as senderStamp" << std::endl;
        std::cerr << "         --name:          name of the shared memory area to attach" << std::endl;
        std::cerr << "         --width:         width of the frame" << std::endl;
        std::cerr << "         --height:        height of the frame" << std::endl;
        std::cerr << "         --gop:           optional: length of group of pictures (default = 10)" << std::endl;
        std::cerr << "         --preset:        optional: one of x264's presets: ultrafast, superfast, veryfast, faster, fast, medium, slow, slower, veryslow (default: veryfast)" << std::endl;
        std::cerr << "         --tune:          optional: one of x264's tunes: film, animation, grain, stillimage, psnr, ssim, fastdecode, zerolatency (default: zerolatency)" << std::endl;
        std::cerr << "         --scenecut:      optional: sets the threshold for I/IDR frame placement (read: scene change detection) (default: 40, 0: disable)" << std::endl;
        std::cerr << "         --intra-refresh: optional: toggle whether or not to use periodic intra refresh instead of IDR frame (default: 0)" << std::endl;
        std::cerr << "         --bframe:        optional: sets the maximum number of concurrent B-frames that x264 can use (default: 3)" << std::endl;
        std::cerr << "         --badapt:        optional: sets the adaptive B-frame placement decision algorithm. (default: 1, 0: disabled, 1: 'Fast' algorithm, 2: 'Optimal' algorithm)" << std::endl;
        std::cerr << "         --cabac:         optional: toggle entropy encoding (default: CAVLC (0))" << std::endl;
        std::cerr << "         --threads        optional: number of threads (default: 1, O: auto, >1: number of theads, max 4)" << std::endl;
        std::cerr << "         --rc-mode        optional: rate control method (default: 1, 0: CQP, 1: CRF, 2: ABR)" << std::endl;
        std::cerr << "         --qp             optional: specifies the P-frame quantizer (default: 23, 0: lossless, min: 0, max 51)" << std::endl;
        std::cerr << "         --qpmin          optional: minimum quantizer that x264 will ever use (default: 0)" << std::endl;
        std::cerr << "         --qpmax          optional: maximum quantizer that x264 will ever use (default: 51)" << std::endl;
        std::cerr << "         --qpstep         optional: maximum change in quantizer between two frames (default: 4)" << std::endl;
        std::cerr << "         --bitrate:       optional: desired bitrate (default: 1,500,000, min: 100,000 max: 5,000,000)" << std::endl;
        std::cerr << "         --crf:           optional: constant ratefactor value (default: 23.0)" << std::endl;
        std::cerr << "         --ipratio:       optional: target average increase in quantizer for I-frames as compared to P-frames (default: 1.4)" << std::endl;
        std::cerr << "         --pbratio:       optional: target average decrease in quantizer for B-frames as compared to P-frames (default: 1.3)" << std::endl;
        std::cerr << "         --aq-mode:       optional: adaptive quantization mode (default: 1, 0: Disable AQ, 1: Enable AQ, 2: Auto-variance AQ)" << std::endl;
        std::cerr << "         --aq-strength:   optional: adaptive quantization strength (default: 1.0, min: 0.0, max: 2.0)" << std::endl;
        std::cerr << "         --weightp:       optional: weighting for P-frames (default: 2, 0: Disabled, 1: Blind offset, 2: Smart analysis with duplicates)" << std::endl;
        std::cerr << "         --me:            optional: full-pixel motion estimation method (default: 1, 0: dia, 1: hex, 2: umh, 3: esa, 4: tesa)" << std::endl;
        std::cerr << "         --merange:       optional: controls the max range of the motion search in pixels (default: 16, min: 4, max: 16)" << std::endl;
        std::cerr << "         --subme:         optional: subpixel estimation complexity (default: 7, 2: QPel SATD 2 iterations, 3: HPel on MB then QPel, 4. Always QPel,\n"
                "                          5: Multi QPel + bi-directional motion estimation, 6: RD on I/P frames, 7: RD on all frames, 8: RD refinement on I/P frames,\n"
                "                          9. RD refinement on all frames, 10: QP-RD (requires --trellis=2, --aq-mode > 0))" << std::endl;
        std::cerr << "         --trellis:       optional: Trellis quantization to increase efficiency (default: 1, 0: disable, 1: Enabled only on the final encode of a macroblock 2: Enabled on all mode decisions)" << std::endl;
        std::cerr << "         --nr:            optional: fast noise reduction (default: 0, 0: disable, min: 0, max: 1000 (100 to 1000 for denoising)" << std::endl;
        std::cerr << "         --verbose:       print encoding information" << std::endl;
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

        //Thesis constants
        const std::string TUNE{(commandlineArguments["tune"].size() != 0) ? commandlineArguments["tune"] : "zerolatency"};
        const uint32_t THREADS{(commandlineArguments["threads"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["threads"])), ZERO), FOUR): 1};

        // Frame-type options
        const uint32_t SCENECUT{(commandlineArguments["scenecut"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["scenecut"])) : 40};
        const uint32_t INTRA_REFRESH{(commandlineArguments["intra-refresh"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["intra-refresh"])) : 0};
        const uint32_t BFRAME{(commandlineArguments["bframe"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["bframe"])) : 3};
        const uint32_t B_ADAPT{(commandlineArguments["badapt"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["badapt"])), ZERO), TWO): 1};
        const uint32_t CABAC{(commandlineArguments["cabac"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["cabac"])), ZERO), ONE): 0};


        //Rate-control
        const uint32_t RC_METHOD{(commandlineArguments["rc-method"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["rc-method"])) : 1};
        const uint32_t QP{(commandlineArguments["qp"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["qp"])), ONE), FIFTYONE): 23};
        const uint32_t QPMIN{(commandlineArguments["qpmin"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["qpmin"])), ZERO), FIFTYONE): 0};
        const uint32_t QPMAX{(commandlineArguments["qpmax"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["qpmax"])), ZERO), FIFTYONE): 51};
        const uint32_t QPSTEP{(commandlineArguments["qpstep"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["qpstep"])) : 4};
        const uint32_t BITRATE_MIN{100000};
        const uint32_t BITRATE_DEFAULT{1500000};
        const uint32_t BITRATE_MAX{5000000};
        const uint32_t BITRATE{(commandlineArguments["bitrate"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["bitrate"])), BITRATE_MIN), BITRATE_MAX) : BITRATE_DEFAULT};
        const float_t CRF{(commandlineArguments["crf"].size() != 0) ? static_cast<float_t >(std::stof(commandlineArguments["crf"])) : float(23.0)};
        const float_t IPRATIO{(commandlineArguments["ipratio"].size() != 0) ? static_cast<float_t >(std::stof(commandlineArguments["ipratio"])) : float(1.4)};
        const float_t PBRATIO{(commandlineArguments["pbratio"].size() != 0) ? static_cast<float_t >(std::stof(commandlineArguments["pbratio"])) : float(1.3)};
        const uint32_t AQ_MODE{(commandlineArguments["aq-mode"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["aq-mode"])), ZERO), TWO) : 1};
        const float_t AQ_STRENGTH{(commandlineArguments["aq-strength"].size() != 0) ? static_cast<float_t >(std::stof(commandlineArguments["aq-strength"])) : float(1.0)};
        const uint32_t WEIGHTP{(commandlineArguments["weightp"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["weightp"])), ZERO), TWO) : 2};
        const uint32_t ME{(commandlineArguments["me"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["me"])), ZERO), FOUR) : 1};
        const uint32_t MERANGE{(commandlineArguments["merange"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["merange"])), FOUR), SIXTEEN) : 16};
        const uint32_t SUBME{(commandlineArguments["subme"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["subme"])), TWO), TEN) : 2};
        const uint32_t TRELLIS{(commandlineArguments["trellis"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["trellis"])), ZERO), TWO) : 1};
        const uint32_t NR{(commandlineArguments["nr"].size() != 0) ? std::min(std::max(static_cast<uint32_t>(std::stoi(commandlineArguments["nr"])), ZERO), THOUSAND) : 0};

        std::unique_ptr<cluon::SharedMemory> sharedMemory(new cluon::SharedMemory{NAME});
        if (sharedMemory && sharedMemory->valid()) {
            std::clog << "[opendlv-video-x264-encoder]: Attached to '" << sharedMemory->name() << "' (" << sharedMemory->size() << " bytes)." << std::endl;

            // Configure x264 parameters.
            x264_param_t parameters;
            if (0 != x264_param_default_preset(&parameters, PRESET.c_str(), TUNE.c_str())) {
                std::cerr << "[opendlv-video-x264-encoder]: Failed to load preset parameters (" << PRESET << "," << TUNE << ") for x264." << std::endl;
                return 1;
            }
            parameters.i_width  = WIDTH;
            parameters.i_height = HEIGHT;
            parameters.i_log_level = (VERBOSE ? X264_LOG_INFO : X264_LOG_NONE);
            parameters.i_csp = X264_CSP_I420;
            parameters.i_bitdepth = 8;
            parameters.i_threads = THREADS;
            parameters.i_keyint_max = GOP;
            parameters.i_fps_num = 20 /* implicitly derived from SharedMemory notifications */;
            parameters.b_vfr_input = 0;
            parameters.b_repeat_headers = 1;
            parameters.b_annexb = 1;

            /*
            * Thesis parameters
            * http://www.chaneru.com/Roku/HLS/X264_Settings.htm
            * https://code.videolan.org/videolan/x264/blob/master/x264.h
            */
            parameters.i_scenecut_threshold = SCENECUT;
            parameters.i_bframe = BFRAME;
            parameters.i_bframe_adaptive = B_ADAPT;
            parameters.b_intra_refresh = INTRA_REFRESH;
            parameters.b_cabac = CABAC;

            parameters.rc.i_rc_method = RC_METHOD;
            parameters.rc.i_qp_constant = QP;
            parameters.rc.i_qp_min = QPMIN;
            parameters.rc.i_qp_max = QPMAX;
            parameters.rc.i_qp_step = QPSTEP;
            parameters.rc.i_bitrate = BITRATE;
            parameters.rc.f_rf_constant_max = CRF;
            parameters.rc.f_ip_factor = IPRATIO;
            parameters.rc.f_pb_factor = PBRATIO;
            parameters.rc.i_aq_mode = AQ_MODE;

            if (AQ_STRENGTH > 2)
                parameters.rc.f_aq_strength = 2;
            else
                parameters.rc.f_aq_strength = AQ_STRENGTH;

            parameters.analyse.i_weighted_pred = WEIGHTP;
            parameters.analyse.i_me_method = ME;
            parameters.analyse.i_me_range = MERANGE;
            parameters.analyse.i_subpel_refine = SUBME;
            parameters.analyse.i_trellis = TRELLIS;
            parameters.analyse.i_noise_reduction = NR;

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
