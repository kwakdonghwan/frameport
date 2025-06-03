// 컴파일시 등록된 TypeName에 맞춰 여러개의 Instance를 생성해주는 Farctory +
// Reigster 패턴의 클래스

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

#ifndef NEXUM_COM_EXTERNAL_BUS_FACTORY_FACTORYREGISTRY_HPP
#define NEXUM_COM_EXTERNAL_BUS_FACTORY_FACTORYREGISTRY_HPP

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

/**
 * @brief 타입별로 Creator(생성자)를 등록하고,
 *       인스턴스 이름을 지정해서 객체를 동적으로 생성할 수 있는 Registry.
 *
 * - 등록: TypeName → Creator
 * - 생성: create(TypeName, InstanceName)
 *   * InstanceName을 생략하면 TypeName=InstanceName으로 생성
 * - contains(): 타입 등록 여부 확인
 * - registeredTypes(): 등록된 타입 목록 조회
 */
template <typename Base>
class FactoryRegistry {
 public:
  // 생성자 함수 시그니처: instanceName을 받아 객체 생성
  using Creator =
      std::function<std::unique_ptr<Base>(const std::string& instanceName)>;

  static FactoryRegistry& instance() {
    static FactoryRegistry registry;
    return registry;
  }

  /**
   * @brief 타입 등록 (TypeName → 생성자)
   * @return 등록 성공 여부(중복 등록 시 false)
   */
  bool registerType(const std::string& typeName, Creator creator) {
    std::lock_guard<std::mutex> lock(mutex_);
    return creators_.emplace(typeName, std::move(creator)).second;
  }

  /**
   * @brief TypeName, InstanceName으로 객체 생성
   * @param typeName     등록된 타입 이름
   * @param instanceName 인스턴스 이름(생략 시 typeName 사용)
   * @return 생성된 객체(unique_ptr)
   */
  std::unique_ptr<Base> create(const std::string& typeName,
                               const std::string& instanceName = "") const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = creators_.find(typeName);
    if (it != creators_.end()) {
      const std::string& realName =
          instanceName.empty() ? typeName : instanceName;
      return (it->second)(realName);
    }
    return nullptr;
  }

  /**
   * @brief 타입 등록 여부 확인
   * @param typeName 등록 여부를 확인할 타입명
   * @return true면 등록됨, false면 미등록
   */
  bool contains(const std::string& typeName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return creators_.find(typeName) != creators_.end();
  }

  /**
   * @brief 등록된 타입명 목록 반환
   */
  std::vector<std::string> registeredTypes() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    result.reserve(creators_.size());
    for (const auto& kv : creators_) result.push_back(kv.first);
    return result;
  }

 private:
  FactoryRegistry() = default;
  FactoryRegistry(const FactoryRegistry&) = delete;
  FactoryRegistry& operator=(const FactoryRegistry&) = delete;

  mutable std::mutex mutex_;
  std::unordered_map<std::string, Creator> creators_;
};

#endif  // NEXUM_COM_EXTERNAL_BUS_FACTORY_FACTORYREGISTRY_HPP
