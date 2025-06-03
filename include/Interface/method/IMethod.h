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

#ifndef NEXUM_COM_INTERFACE_IMETHOD_H
#define NEXUM_COM_INTERFACE_IMETHOD_H
#include <any>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief 동적 메서드 호출 및 등록을 지원하는 인터페이스 클래스
 *
 * IMethod는 메서드명을 문자열로 식별하고, 다양한 시그니처의 함수(람다,
 * 프리함수, 멤버함수 등)를 등록하여 std::any 기반으로 동적 호출을 지원합니다.
 * 멀티스레드 환경에서도 안전하게 사용할 수 있도록 mutex로 보호됩니다.
 */
class IMethod {
 public:
  /**
   * @brief 메서드 함수 타입
   *
   * std::vector<std::any>를 파라미터로 받아 std::any를 반환하는 함수 객체 정의
   */
  using MethodFn = std::function<std::any(const std::vector<std::any>&)>;

  /**
   * @brief 가상 소멸자
   */
  virtual ~IMethod() = default;

  /**
   * @brief 메서드명을 통해 메서드를 호출합니다.
   * @param methodName 호출할 메서드명 (string)
   * @param args std::any 파라미터 벡터 (기본값: 빈 벡터)
   * @return std::any 메서드의 반환값
   * @throws std::runtime_error 해당 메서드가 등록되지 않았을 때
   */
  virtual std::any invoke(const std::string& methodName,
                          const std::vector<std::any>& args = {}) {
    std::lock_guard<std::mutex> lock(method_mutex_);
    auto it = methods_.find(methodName);
    if (it == methods_.end())
      throw std::runtime_error("IMethod: method '" + methodName +
                               "' not registered.");
    return it->second(args);
  }

  /**
   * @brief 메서드를 등록합니다. (람다, 프리함수, 멤버함수 등 모두 지원)
   * @tparam F 함수 객체 타입 (임의)
   * @param methodName 등록할 메서드명
   * @param func 등록할 함수 또는 람다
   */
  template <typename F>
  void registerMethod(const std::string& methodName, F&& func) {
    std::lock_guard<std::mutex> lock(method_mutex_);
    methods_[methodName] = MethodFn(std::forward<F>(func));
  }

  /**
   * @brief 등록된 모든 메서드명을 반환합니다.
   * @return std::vector<std::string> 메서드명 리스트
   */
  std::vector<std::string> methodList() const {
    std::lock_guard<std::mutex> lock(method_mutex_);
    std::vector<std::string> list;
    for (const auto& kv : methods_) list.push_back(kv.first);
    return list;
  }

 protected:
  /** @brief 메서드 동기화를 위한 mutex */
  mutable std::mutex method_mutex_;
  /** @brief 메서드명과 함수 객체 매핑 테이블 */
  std::unordered_map<std::string, MethodFn> methods_;
};

#endif  // #ifndef NEXUM_COM_INTERFACE_IMETHOD_H