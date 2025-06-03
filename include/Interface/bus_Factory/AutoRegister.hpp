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

#ifndef NEXUM_COM_EXTERNAL_BUS_FACTORY_AUTOREGISTER_HPP
#define NEXUM_COM_EXTERNAL_BUS_FACTORY_AUTOREGISTER_HPP

#include <memory>
#include <string>

#include "FactoryRegistry.hpp"

/**
 * @brief 자동 타입 등록을 위한 템플릿 CRTP 구조체.
 *
 * 이 구조체는 CRTP(Curiously Recurring Template Pattern)를 활용하여,
 * Derived 타입이 선언될 때 FactoryRegistry<Base>에 자동으로 등록됩니다.
 * 이를 통해 객체의 동적 생성 및 타입 등록 과정을 자동화할 수 있습니다.
 *
 * @tparam Derived 실제 등록될 파생 클래스 타입
 * @tparam Base    공통의 베이스 클래스 타입 (FactoryRegistry에 등록)
 */
template <typename Derived, typename Base>
struct AutoRegister : Base {
  /**
   * @brief 이름을 받아 Derived 타입의 인스턴스를 동적 생성합니다.
   * @param name 객체 생성 시 사용할 이름
   * @return std::unique_ptr<Base> 생성된 객체의 베이스 클래스 포인터
   */
  static std::unique_ptr<Base> createInstance(const std::string& name) {
    return std::make_unique<Derived>(name);
  }

  /**
   * @brief 정적 타입 이름을 반환합니다.
   * @return std::string 파생 클래스에서 제공하는 타입 이름
   */
  static std::string staticName() { return Derived::staticName(); }

  /**
   * @brief 타입 등록을 위한 정적 플래그.
   *
   * 클래스가 처음 참조될 때 FactoryRegistry<Base>에 Derived 타입을 자동
   * 등록합니다.
   */
  static bool registered_;
};

/**
 * @brief AutoRegister의 정적 멤버 초기화 및 타입 등록 구현.
 *
 * 클래스가 로드될 때 FactoryRegistry에 Derived 타입을 등록합니다.
 */
template <typename Derived, typename Base>
inline bool AutoRegister<Derived, Base>::registered_ = []() -> bool {
  FactoryRegistry<Base>::instance().registerType(
      Derived::staticName(), &AutoRegister<Derived, Base>::createInstance);
  return true;
}();
#endif