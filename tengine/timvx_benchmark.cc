/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * License); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/*
 * Copyright (c) 2021, OPEN AI LAB
 * Author: qtang@openailab.com
 * Update: lswang@openailab.com
 */

#include "tengine/c_api.h"
#include "common/cmdline.hpp"
#include "common/timer.hpp"
#include "../examples/common/common.h"
#include <cstdio>
#include <string>

#define DEFAULT_THREAD_COUNT 1
#define DEFAULT_CPU_AFFINITY 255
#define DEFAULT_IMG_H        224
#define DEFAULT_IMG_W        224
#define DEFAULT_IMG_BATCH    1
#define DEFAULT_IMG_CHANNEL  3
#define DEFAULT_LOOP_COUNT   1

int benchmark_graph(options_t* opt, const char* name, const char* file, int height, int width, int channel, int batch, int benchmark_loop)
{
    /* create VeriSilicon TIM-VX backend */
    context_t timvx_context = create_context("timvx", 1);
    int rtt = set_context_device(timvx_context, "TIMVX", NULL, 0);
    if (0 > rtt)
    {
        fprintf(stderr, " add_context_device VSI DEVICE failed.\n");
        return -1;
    }

    // create graph, load tengine model xxx.tmfile
    graph_t graph = create_graph(timvx_context, "tengine", file);
    if (nullptr == graph)
    {
        fprintf(stderr, "Tengine Benchmark: Create graph failed.\n");
        return -1;
    }

    // set the input shape to initial the graph, and pre-run graph to infer shape
    int input_size = height * width * channel;
    int shape[] = { batch, channel, height, width };    // nchw

    std::vector<uint8_t> inout_buffer(input_size);

    memset(inout_buffer.data(), 1, inout_buffer.size() * sizeof(uint8_t));

    tensor_t input_tensor = get_graph_input_tensor(graph, 0, 0);
    if (input_tensor == nullptr)
    {
        fprintf(stderr, "Tengine Benchmark: Get input tensor failed.\n");
        return -1;
    }

    if (set_tensor_shape(input_tensor, shape, 4) < 0)
    {
        fprintf(stderr, "Tengine Benchmark: Set input tensor shape failed.\n");
        return -1;
    }

    if (prerun_graph_multithread(graph, *opt) < 0)
    {
        fprintf(stderr, "Tengine Benchmark: Pre-run graph failed.\n");
        return -1;
    }

    // prepare process input data, set the data mem to input tensor
    if (set_tensor_buffer(input_tensor, inout_buffer.data(), (int)(inout_buffer.size() * sizeof(uint8_t))) < 0)
    {
        fprintf(stderr, "Tengine Benchmark: Set input tensor buffer failed\n");
        return -1;
    }

    // warning up graph
    if (run_graph(graph, 1) < 0)
    {
        fprintf(stderr, "Tengine Benchmark: Run graph failed.\n");
        return -1;
    }

    std::vector<float> time_cost(benchmark_loop);

    // run graph
    for (int i = 0; i < benchmark_loop; i++)
    {
        Timer timer;
        int ret = run_graph(graph, 1);
        time_cost[i] = timer.TimeCost();

        if (0 != ret)
        {
            fprintf(stderr, "Tengine Benchmark: Run graph failed\n");
            return -1;
        }
    }

    float min = time_cost[0], max = time_cost[0], sum = 0.f;
    for (const auto& var : time_cost)
    {
        if (min > var)
        {
            min = var;
        }

        if (max < var)
        {
            max = var;
        }

        sum += var;
    }
    sum /= (float)time_cost.size();
    fprintf(stderr, "%20s  min = %7.2f ms   max = %7.2f ms   avg = %7.2f ms\n", name, min, max, sum);

    // release tengine graph
    release_graph_tensor(input_tensor);
    postrun_graph(graph);
    destroy_graph(graph);

    return 0;
}


int main(int argc, char* argv[])
{
    // TIM-VX backends
    struct options opt;
    opt.num_thread = DEFAULT_THREAD_COUNT;
    opt.cluster = TENGINE_CLUSTER_ALL;
    opt.precision = TENGINE_MODE_UINT8;
    opt.affinity = DEFAULT_CPU_AFFINITY;
    // basic param
    int loop_count = DEFAULT_LOOP_COUNT;
    int num_thread = DEFAULT_THREAD_COUNT;
    int cpu_affinity = DEFAULT_CPU_AFFINITY;
    int img_h = DEFAULT_IMG_H;
    int img_w = DEFAULT_IMG_W;
    int img_n = DEFAULT_IMG_BATCH;
    int img_c = DEFAULT_IMG_CHANNEL;
    char* model_file = NULL;
    float img_hw[4] = {0.f};
    // arg parse
    int res;
    while ((res = getopt(argc, argv, "m:i:l:g:s:w:r:t:a:h")) != -1)
    {
        switch (res)
        {
        case 'm':
            model_file = optarg;
            break;
        case 'r':
            loop_count = atoi(optarg);
            break;
        case 't':
            num_thread = atoi(optarg);
            break;
        case 'a':
            cpu_affinity = atoi(optarg);
            break;
        case 'g':
            split(img_hw, optarg, ",");
            img_n = (int)img_hw[0];
            img_c= (int)img_hw[1];
            img_h = (int)img_hw[2];
            img_w = (int)img_hw[3];
            break;
        default:
            break;
        }
    }

    if (model_file == NULL)
    {
        fprintf(stderr, "Error: Tengine model file not specified!\n");
        return -1;
    }

    fprintf(stdout, "Tengine-lite library version: %s\n", get_tengine_version());
    benchmark_graph(&opt, model_file, model_file, img_h, img_w, img_c, img_n, loop_count);
    release_tengine();
    fprintf(stderr, "ALL TEST DONE.\n");    

    return 0;
}