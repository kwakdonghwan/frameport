// 본 소스코드는 BSD 3-Clause 라이선스를 따릅니다.
// This file is licensed under the BSD 3-Clause License.
// 개인프로젝트 코드이며 수정, 배포, 상업적이용은 자유로우나 상단 주석을 제거시
// 저자권 위반입니다.
/*
 * Copyright (c) 2025, 곽동환 <arbiter1225@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <any>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "com/external/Interface/interface.h"

// --- 1. Trivially Copyable 데이터 정의 ---
struct MyData {
  int value;
  double timestamp;
} __attribute__((packed));

// --- 2. Frame 구현 (instanceName, id() 자동) ---
class FrameImpl : public FrameBase<MyData, FrameImpl> {
 public:
  using FrameBase::FrameBase;  // Base 생성자 상속 (instanceName 자동)

  static std::string staticName() { return "FrameImpl"; }
  size_t size() const override { return sizeof(MyData); }

  FrameImpl(const std::string& instanceName) : FrameBase(instanceName) {
    // 신호 등록
    registerSignal("value", &MyData::value, &data_, &data_rwlock_);
    registerSignal("timestamp", &MyData::timestamp, &data_, &data_rwlock_);
  }
};

// --- 3. Port 구현 (name() 자동, type()/open()/close()만 오버라이드) ---
class PortServer : public PortBase<PortServer> {
 public:
  using PortBase::PortBase;  // Base 생성자 상속 (instanceName 자동)
  static std::string staticName() { return "PortServer"; }
  std::string type() const override { return "server"; }
  bool open() override {
    std::cout << "[PortServer] Opened!\n";
    return true;
  }
  void close() override { std::cout << "[PortServer] Closed!\n"; }
};

class PortClient : public PortBase<PortClient> {
 public:
  using PortBase::PortBase;
  static std::string staticName() { return "PortClient"; }
  std::string type() const override { return "client"; }
  bool open() override {
    std::cout << "[PortClient] Opened!\n";
    return true;
  }
  void close() override { std::cout << "[PortClient] Closed!\n"; }
};

inline void registerAllFrameTypes() {
  (void)AutoRegister<FrameImpl, IFrame>::registered_;
  (void)AutoRegister<PortServer, IPort>::registered_;
  (void)AutoRegister<PortClient, IPort>::registered_;
}

// --- 4. 예제 메인 ---
int main() {
  registerAllFrameTypes();
  // [2] Frame 인스턴스 1개 생성 & FrameBus에 등록
  auto frame =
      FactoryRegistry<IFrame>::instance().create("FrameImpl", "SharedFrame");
  FrameBus::instance().registerFrame("SharedFrame",
                                     std::shared_ptr<IFrame>(std::move(frame)));

  // [3] PortServer 생성 & 프레임 연결
  auto portServer =
      FactoryRegistry<IPort>::instance().create("PortServer", "Server");
  auto* pServer = portServer.get();
  pServer->open();
  pServer->connectFrame("SharedFrame");

  // [4] PortClient 2개 생성 & 프레임 연결
  auto portClient1 =
      FactoryRegistry<IPort>::instance().create("PortClient", "Client1");
  auto portClient2 =
      FactoryRegistry<IPort>::instance().create("PortClient", "Client2");
  portClient1->open();
  portClient2->open();
  portClient1->connectFrame("SharedFrame");
  portClient2->connectFrame("SharedFrame");

  // [5] PortClient1이 SharedFrame의 value/timestamp 신호를 set
  portClient1->setSignalToFrame("SharedFrame", "value", 42);
  portClient1->setSignalToFrame("SharedFrame", "timestamp", 3.1415);

  // [6] PortClient2가 SharedFrame에서 value, timestamp를 읽음
  int val = portClient2->getSignalFromFrame<int>("SharedFrame", "value");
  double ts =
      portClient2->getSignalFromFrame<double>("SharedFrame", "timestamp");
  std::cout << "[Client2] value: " << val << ", timestamp: " << ts << std::endl;

  // [7] PortServer에서 SharedFrame에 콜백 등록
  uint64_t cbid =
      pServer->subscribeFrame("SharedFrame", [](const char* buf, size_t sz) {
        if (sz == sizeof(MyData)) {
          const MyData* data = reinterpret_cast<const MyData*>(buf);
          std::cout << "[Server] Callback! value: " << data->value
                    << ", timestamp: " << data->timestamp << std::endl;
        }
      });

  // [8] PortClient1이 setSignalToFrameWithPublish로 값을 바꿈 → 콜백 발동
  portClient1->setSignalToFrameWithPublish("SharedFrame", "value", 99);

  // [9] rawData로 프레임 값 직접 카피 (get/setRawDataToFrame,
  // getRawDataFromFrame)
  MyData tmp;
  portClient1->getRawDataFromFrame("SharedFrame",
                                   [&](const char* buf, size_t sz) {
                                     if (sz == sizeof(MyData))
                                       std::memcpy(&tmp, buf, sz);
                                   });
  std::cout << "[Client1] Fetched RawData: value=" << tmp.value
            << ", timestamp=" << tmp.timestamp << std::endl;

  tmp.value = 777;
  tmp.timestamp = 1.23;
  portClient2->setRawDataToFrameWithPublish(
      "SharedFrame", reinterpret_cast<const char*>(&tmp), sizeof(MyData));

  // [10] PortServer 콜백 해제
  pServer->unsubscribeFrame(cbid);

  // [11] 종료
  portClient1->close();
  portClient2->close();
  pServer->close();
}