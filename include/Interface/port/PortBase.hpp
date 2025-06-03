// 본 소스코드는 BSD 3-Clause 라이선스를 따릅니다.
// This file is licensed under the BSD 3-Clause License.
// 개인프로젝트 코드이며 수정, 배포, 상업적이용은 자유로우나 상단 주석을 제거시
// 저작권 보호법 위반입니다.
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

#ifndef NEXUM_COM_EXTERNAL_INTERFACE_PORTBASE_HPP
#define NEXUM_COM_EXTERNAL_INTERFACE_PORTBASE_HPP

#include <any>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "../bus_Factory/AutoRegister.hpp"
#include "../bus_Factory/FrameBus.hpp"
#include "../frame/IFrame.h"
#include "../port/IPort.h"

/**
 * @brief IPort 기반 외부 접점(Port) 구현을 위한 템플릿 베이스 클래스
 *
 * PortBase는 CRTP(AutoRegister<Derived, IPort>)를 활용해 파생 클래스가
 * 자동으로 등록되도록 하며, 기본적인 프레임 연결/데이터 송수신/콜백 구독
 * 관리 기능을 제공합니다. 모든 Port 구현체는 이 클래스를 상속받아
 * 최소한의 코드로 일관된 프레임 접근 및 신호 전달/조회가 가능합니다.
 * @tparam Derived 실제 구현 포트 클래스 타입
 */
template <typename Derived>
class PortBase : public AutoRegister<Derived, IPort> {
 public:
  /** @brief IFrame 스마트 포인터 타입 정의 */
  using FramePtr = std::shared_ptr<IFrame>;

  /**
   * @brief 생성자 - 인스턴스 이름으로 포트 객체를 생성 및 등록
   * @param instanceName 포트 인스턴스명 (프레임 버스 등에서 식별자)
   */
  explicit PortBase(const std::string& instanceName);

  /**
   * @brief 포트 인스턴스 이름 반환
   * @return std::string 인스턴스명
   */
  std::string name() const override;

  // ------------------- 프레임 연결/해제 -------------------

  /**
   * @brief 지정된 프레임을 포트에 연결
   * @param frameName 연결할 프레임 이름
   * @return 성공 여부
   */
  bool connectFrame(const std::string& frameName) override;

  /**
   * @brief 지정된 프레임을 포트에서 해제
   * @param frameName 해제할 프레임 이름
   */
  void disconnectFrame(const std::string& frameName) override;

  // ------------------- 신호/데이터 입력 -------------------

  /**
   * @brief 프레임의 시그널에 값 입력 (Publish 없음)
   * @param frameName 프레임 이름
   * @param signal 시그널명
   * @param value 입력값 (std::any)
   * @return 성공 여부
   */
  bool setSignalToFrame(const std::string& frameName, const std::string& signal,
                        const std::any& value) override;

  /**
   * @brief 프레임의 시그널에 값 입력 및 즉시 Publish
   * @param frameName 프레임 이름
   * @param signal 시그널명
   * @param value 입력값 (std::any)
   * @return 성공 여부
   */
  bool setSignalToFrameWithPublish(const std::string& frameName,
                                   const std::string& signal,
                                   const std::any& value) override;

  /**
   * @brief 프레임에 Raw 데이터 입력 (Publish 없음)
   * @param frameName 프레임 이름
   * @param data 데이터 포인터
   * @param size 데이터 길이 (바이트)
   * @return 성공 여부
   */
  bool setRawDataToFrame(const std::string& frameName, const char* data,
                         size_t size) override;

  /**
   * @brief 프레임에 Raw 데이터 입력 및 즉시 Publish
   * @param frameName 프레임 이름
   * @param data 데이터 포인터
   * @param size 데이터 길이 (바이트)
   * @return 성공 여부
   */
  bool setRawDataToFrameWithPublish(const std::string& frameName,
                                    const char* data, size_t size) override;

  // ------------------- 신호/데이터 조회 -------------------

  /**
   * @brief 프레임의 시그널 값을 std::any로 조회
   * @param frameName 프레임 이름
   * @param signal 시그널명
   * @return std::any 값
   */
  std::any getSignalFromFrameAsAny(const std::string& frameName,
                                   const std::string& signal) override;

  /**
   * @brief 프레임의 Raw 데이터를 콜백으로 읽기
   * @param frameName 프레임 이름
   * @param cb (data, size) 형태의 콜백 함수
   * @return 성공 여부
   */
  bool getRawDataFromFrame(
      const std::string& frameName,
      std::function<void(const char*, size_t)> cb) override;

  // ------------------- 콜백 구독/해제 -------------------

  /**
   * @brief 프레임 데이터 콜백 구독 (비동기/스레드 분리 방식)
   * @param frameName 프레임 이름
   * @param cb 데이터 수신시 호출될 콜백
   * @return uint64_t 콜백 인스턴스 ID
   */
  uint64_t subscribeFrame(const std::string& frameName,
                          std::function<void(const char*, size_t)> cb) override;

  /**
   * @brief 프레임 데이터 콜백 구독 (Direct: 호출 스레드에서 직접 호출)
   * @param frameName 프레임 이름
   * @param cb 데이터 수신시 호출될 콜백
   * @return uint64_t 콜백 인스턴스 ID
   */
  uint64_t subscribeFrameDirect(
      const std::string& frameName,
      std::function<void(const char*, size_t)> cb) override;

  /**
   * @brief 프레임 콜백 구독 해제
   * @param callbackId 구독시 반환받은 콜백 인스턴스 ID
   */
  void unsubscribeFrame(uint64_t callbackId) override;

 protected:
  /**
   * @brief 등록된 프레임을 이름으로 찾아 반환 (없으면 nullptr)
   * @param name 프레임 이름
   * @return std::shared_ptr<IFrame> 프레임 포인터
   */
  std::shared_ptr<IFrame> findFrame(const std::string& name) const;

  /** @brief 포트 인스턴스 이름 */
  std::string instanceName_;

  /** @brief 프레임 관리 (프레임 이름 ↔ 포인터), 외부 노출 금지 */
  mutable std::mutex frames_mutex_;
  std::unordered_map<std::string, std::shared_ptr<IFrame>> frames_;

  /** @brief 콜백 관리 (구독 해제용) */
  mutable std::mutex cb_mutex_;
  std::unordered_map<uint64_t, std::shared_ptr<IFrame>> callback_map_;
};

// -- 구현부 -- //

template <typename Derived>
inline PortBase<Derived>::PortBase(const std::string& instanceName)
    : instanceName_(instanceName) {}

template <typename Derived>
inline std::string PortBase<Derived>::name() const {
  return instanceName_;
}

template <typename Derived>
inline bool PortBase<Derived>::connectFrame(const std::string& frameName) {
  std::lock_guard<std::mutex> lock(frames_mutex_);
  if (frames_.count(frameName)) return true;  // 이미 연결
  auto frame = FrameBus::instance().getFrame(frameName);
  if (!frame) return false;
  frames_[frameName] = frame;
  return true;
}

template <typename Derived>
inline void PortBase<Derived>::disconnectFrame(const std::string& frameName) {
  std::lock_guard<std::mutex> lock(frames_mutex_);
  frames_.erase(frameName);
}

template <typename Derived>
inline bool PortBase<Derived>::setSignalToFrame(const std::string& frameName,
                                                const std::string& signal,
                                                const std::any& value) {
  auto frame = findFrame(frameName);
  if (!frame) return false;
  try {
    frame->setSignal(signal, value);
    return true;
  } catch (...) {
    return false;
  }
}

template <typename Derived>
inline bool PortBase<Derived>::setSignalToFrameWithPublish(
    const std::string& frameName, const std::string& signal,
    const std::any& value) {
  auto frame = findFrame(frameName);
  if (!frame) return false;
  try {
    frame->setSignalWithPublish(signal, value);
    return true;
  } catch (...) {
    return false;
  }
}

template <typename Derived>
inline bool PortBase<Derived>::setRawDataToFrame(const std::string& frameName,
                                                 const char* data,
                                                 size_t size) {
  auto frame = findFrame(frameName);
  if (!frame) return false;
  bool ok = false;
  frame->writeRawData([&](char* buf, size_t bufsize) {
    if (size == bufsize) {
      std::memcpy(buf, data, size);
      ok = true;
    }
  });
  return ok;
}

template <typename Derived>
inline bool PortBase<Derived>::setRawDataToFrameWithPublish(
    const std::string& frameName, const char* data, size_t size) {
  auto frame = findFrame(frameName);
  if (!frame) return false;
  bool ok = false;
  frame->writeRawData([&](char* buf, size_t bufsize) {
    if (size == bufsize) {
      std::memcpy(buf, data, size);
      ok = true;
    }
  });
  if (ok) frame->notifyCallbacks();
  return ok;
}

template <typename Derived>
inline std::any PortBase<Derived>::getSignalFromFrameAsAny(
    const std::string& frameName, const std::string& signal) {
  auto frame = findFrame(frameName);
  if (!frame) throw std::runtime_error("Frame not found");
  return frame->getSignal(signal);
}

template <typename Derived>
inline bool PortBase<Derived>::getRawDataFromFrame(
    const std::string& frameName, std::function<void(const char*, size_t)> cb) {
  auto frame = findFrame(frameName);
  if (!frame) return false;
  frame->readRawData(cb);
  return true;
}

template <typename Derived>
inline uint64_t PortBase<Derived>::subscribeFrame(
    const std::string& frameName, std::function<void(const char*, size_t)> cb) {
  auto frame = findFrame(frameName);
  if (!frame) return 0;
  // Threaded 정책: 반드시 addSnapshotCallback 사용!
  auto wrapper = [cb](const std::vector<uint8_t>& data, size_t sz) {
    cb(reinterpret_cast<const char*>(data.data()), sz);
  };
  uint64_t id = frame->addSnapshotCallback(wrapper);
  {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    callback_map_[id] = frame;
  }
  return id;
}

template <typename Derived>
inline uint64_t PortBase<Derived>::subscribeFrameDirect(
    const std::string& frameName, std::function<void(const char*, size_t)> cb) {
  auto frame = findFrame(frameName);
  if (!frame) return 0;
  auto wrapper = [cb](const IFrame& f) { f.readRawData(cb); };
  uint64_t id = frame->addCallback(wrapper, CallbackPolicy::Direct);
  {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    callback_map_[id] = frame;
  }
  return id;
}

template <typename Derived>
inline void PortBase<Derived>::unsubscribeFrame(uint64_t callbackId) {
  std::shared_ptr<IFrame> frame;
  {
    std::lock_guard<std::mutex> lock(cb_mutex_);
    auto it = callback_map_.find(callbackId);
    if (it != callback_map_.end()) {
      frame = it->second;
      callback_map_.erase(it);
    }
  }
  if (frame) frame->removeCallback(callbackId);
}

template <typename Derived>
inline std::shared_ptr<IFrame> PortBase<Derived>::findFrame(
    const std::string& name) const {
  std::lock_guard<std::mutex> lock(frames_mutex_);
  auto it = frames_.find(name);
  if (it != frames_.end()) return it->second;
  return nullptr;
}
#endif  // NEXUM_COM_EXTERNAL_INTERFACE_PORTBASE_HPP