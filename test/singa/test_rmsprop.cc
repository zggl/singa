/************************************************************
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
*************************************************************/

#include "gtest/gtest.h"
#include "singa/model/optimizer.h"
#include <cmath>

TEST(RMSProp, ApplyCPU) {
  singa::RMSProp rmsprop;
  float lr = 0.1f;
  float rho = 0.9f;
  const float v[4] = {0.1f, 0.2f, 0.3f, 0.4f};
  const float g[4] = {0.01f, 0.02f, 0.03f, 0.04f};

  singa::OptimizerConf conf;
  conf.set_rho(rho);
  conf.set_delta(1E-8f);

  singa::Tensor value(singa::Shape{4}), grad(singa::Shape{4});
  value.CopyDataFromHostPtr(v, 4);
  grad.CopyDataFromHostPtr(g, 4);

  rmsprop.Setup(conf);
  rmsprop.Apply(0, lr, "xx", grad, value);

  singa::Tensor v1 = value.Clone();
  const float* newv1 = v1.data<float>();
  float history[4];
  for (int i = 0; i < 4; ++i) history[i] = g[i] * g[i] * (1 - rho);
  for (int i = 0; i < 4; ++i)
    EXPECT_NEAR(newv1[i], v[i] - g[i] * lr / sqrt(history[i] + (float)1E-8),
                1e-5);

  grad.CopyDataFromHostPtr(g, 4);
  rmsprop.Apply(1, lr, "xx", grad, value);
  singa::Tensor v2 = value.Clone();
  const float* newv2 = v2.data<float>();
  for (int i = 0; i < 4; ++i)
    history[i] = history[i] * rho + g[i] * g[i] * (1 - rho);

  for (int i = 0; i < 4; ++i)
    EXPECT_NEAR(newv2[i], newv1[i] - lr * g[i] / sqrt(history[i] + (float)1E-8),
                1e-5);
}

#ifdef USE_CUDA
TEST(RMSProp, ApplyCUDA) {
  singa::RMSProp rmsprop;
  float lr = 0.1f;
  float rho = 0.02;
  const float v[4] = {0.1, 0.2, 0.3, 0.4};
  const float g[4] = {0.01, 0.02, 0.03, 0.04};

  singa::OptimizerConf conf;
  conf.set_rho(rho);
  conf.set_delta(1e-8);

  auto dev = std::make_shared<singa::CudaGPU>();
  singa::Tensor value(singa::Shape{4}, dev), grad(singa::Shape{4}, dev);
  value.CopyDataFromHostPtr(v, 4);
  grad.CopyDataFromHostPtr(g, 4);

  rmsprop.Setup(conf);
  rmsprop.Apply(0, lr, "xx", grad, value);

  singa::Tensor v1 = value.Clone();
  v1.ToHost();
  const float* newv1 = v1.data<float>();
  float history[4];
  for (int i = 0; i < 4; ++i) history[i] = g[i] * g[i] * (1 - rho);
  for (int i = 0; i < 4; ++i)
    EXPECT_NEAR(newv1[i], v[i] - lr * g[i] / sqrt(history[i] + conf.delta()),
                1e-5);

  grad.CopyDataFromHostPtr(g, 4);
  rmsprop.Apply(1, lr, "xx", grad, value);
  singa::Tensor v2 = value.Clone();
  v2.ToHost();
  const float* newv2 = v2.data<float>();
  for (int i = 0; i < 4; ++i)
    history[i] = history[i] * rho + g[i] * g[i] * (1 - rho);

  for (int i = 0; i < 4; ++i)
    EXPECT_NEAR(newv2[i],
                newv1[i] - lr * g[i] / sqrt(history[i] + conf.delta()), 1e-5);
}
#endif
