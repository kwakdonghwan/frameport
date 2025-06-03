// 생성된 프레임에 대한 모든 관리 Bus.

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

#ifndef NEXUM_COM_EXTERNAL_BUS_FACTORY_FRAMEBUS_HPP
#define NEXUM_COM_EXTERNAL_BUS_FACTORY_FRAMEBUS_HPP

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

class IFrame;

/**
 * @brief IFrame 객체의 싱글톤 레지스트리(버스) 역할을 하는 클래스
 *
 * FrameBus는 이름 기반으로 IFrame 객체를 등록/조회/삭제/순회할 수 있는
 * 싱글톤 레지스트리입니다. 멀티스레드 환경에서도 안전하게 사용할 수 있도록
 * 내부적으로 mutex로 보호됩니다.
 */
class FrameBus {
 public:
  /**
   * @brief FrameBus의 전역 싱글톤 인스턴스를 반환합니다.
   * @return FrameBus& 싱글톤 객체 참조
   */
  static FrameBus& instance() {
    static FrameBus bus;
    return bus;
  }

  /**
   * @brief 프레임을 이름으로 등록합니다. (기존 이름이 있으면 덮어쓰기)
   * @param name 프레임 식별자
   * @param frame 등록할 IFrame 객체 (shared_ptr)
   */
  void registerFrame(const std::string& name, std::shared_ptr<IFrame> frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    frames_[name] = std::move(frame);
  }

  /**
   * @brief 이름으로 프레임을 조회합니다.
   * @param name 프레임 식별자
   * @return std::shared_ptr<IFrame> 찾은 경우 프레임 포인터, 없으면 nullptr
   */
  std::shared_ptr<IFrame> getFrame(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = frames_.find(name);
    if (it != frames_.end()) return it->second;
    return nullptr;
  }

  /**
   * @brief 이름으로 등록된 프레임을 삭제합니다.
   * @param name 프레임 식별자
   */
  void unregisterFrame(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    frames_.erase(name);
  }

  /**
   * @brief 등록된 모든 프레임에 대해 콜백을 수행합니다.
   * @param cb (프레임 이름, 프레임 객체)로 호출되는 함수/람다
   */
  void forEach(const std::function<void(const std::string&,
                                        std::shared_ptr<IFrame>)>& cb) const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& kv : frames_) {
      cb(kv.first, kv.second);
    }
  }

 private:
  /**
   * @brief FrameBus의 private 생성자 (싱글톤 패턴)
   */
  FrameBus() = default;
  FrameBus(const FrameBus&) = delete;
  FrameBus& operator=(const FrameBus&) = delete;

  /** @brief 동기화를 위한 mutex */
  mutable std::mutex mutex_;
  /** @brief 이름 기반 IFrame 객체 레지스트리 */
  std::unordered_map<std::string, std::shared_ptr<IFrame>> frames_;
};

#endif