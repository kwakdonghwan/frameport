// trivial copy able한 데이터에 대한 CRTP 자동지원

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

#ifndef NEXUM_EXTERNAL_INTERFACE_FRAMEBASE_HPP
#define NEXUM_EXTERNAL_INTERFACE_FRAMEBASE_HPP

#include <cstring>
#include <functional>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#include "../bus_Factory/AutoRegister.hpp"
#include "../frame/IFrame.h"

template <typename T>
concept TriviallyCopyable = std::is_trivially_copyable_v<T>;

/**
 * @brief 데이터 구조체 기반 프레임 템플릿 클래스
 * @tparam DataT 신호 데이터 구조체 타입
 * @tparam Derived CRTP 파생 타입
 */
template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
class FrameBase : public AutoRegister<Derived, IFrame> {
 public:
  using Data = DataT;

  /**
   * @brief 생성자 (데이터 0 초기화 + instanceName)
   */
  explicit FrameBase(const std::string& instanceName);

  /**
   * @brief 데이터 const 참조 반환
   * @return const Data&
   */
  const Data& data() const;

  /**
   * @brief 데이터 참조 반환 (쓰기용)
   * @return Data&
   */
  Data& data();

  /**
   * @brief 안전한 원시 데이터 접근(RAII 람다)
   */
  void readRawData(
      std::function<void(const char*, size_t)> func) const override;
  void writeRawData(std::function<void(char*, size_t)> func) override;

  /**
   * @brief 데이터 크기 반환
   * @return 크기 (바이트)
   */
  size_t size() const override;

  /**
   * @brief 커스텀 직렬화 함수 지정
   * @param s 직렬화 함수
   */
  void setSerializer(std::function<std::vector<uint8_t>(const Data&)> s);

  /**
   * @brief 커스텀 역직렬화 함수 지정
   * @param d 역직렬화 함수
   */
  void setDeserializer(
      std::function<void(Data&, const std::vector<uint8_t>&)> d);

  /**
   * @brief 데이터 직렬화
   * @return 직렬화 데이터
   */
  std::vector<uint8_t> serialize() const override;

  /**
   * @brief 역직렬화 후 콜백 알림
   * @param raw 직렬화 데이터
   * @return 성공 여부
   */
  bool deserializeWithPublish(const std::vector<uint8_t>& raw) override;

  /**
   * @brief 데이터 역직렬화 (콜백 없음)
   * @param raw 직렬화 데이터
   */
  void deserialize(const std::vector<uint8_t>& raw) override;

  /**
   * @brief 프레임 인스턴스 이름 반환
   */
  std::string id() const override;

 protected:
  Data data_;                              ///< 데이터 구조체
  mutable std::shared_mutex data_rwlock_;  ///< 데이터 락(RW)
  std::function<std::vector<uint8_t>(const Data&)>
      serializer_;  ///< 직렬화 함수
  std::function<void(Data&, const std::vector<uint8_t>&)>
      deserializer_;          ///< 역직렬화 함수
  std::string instanceName_;  ///< 인스턴스 이름

  const char* rawData() const override;
  char* rawData() override;
  size_t rawDataSize() const override;
};

// ----- FrameBase<DataT,Derived> 구현 -----

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline FrameBase<DataT, Derived>::FrameBase(const std::string& instanceName)
    : instanceName_(instanceName) {
  std::memset(&data_, 0, sizeof(DataT));
  serializer_ = [](const Data& d) {
    std::vector<uint8_t> buf(sizeof(DataT));
    std::memcpy(buf.data(), &d, sizeof(DataT));
    return buf;
  };
  deserializer_ = [](Data& d, const std::vector<uint8_t>& buf) {
    if (buf.size() != sizeof(DataT))
      throw std::runtime_error("FrameBase: deserialize size mismatch: got " +
                               std::to_string(buf.size()) + ", expected " +
                               std::to_string(sizeof(DataT)));
    std::memcpy(&d, buf.data(), sizeof(DataT));
  };
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline const typename FrameBase<DataT, Derived>::Data&
FrameBase<DataT, Derived>::data() const {
  std::shared_lock<std::shared_mutex> lock(data_rwlock_);
  return data_;
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline typename FrameBase<DataT, Derived>::Data&
FrameBase<DataT, Derived>::data() {
  std::unique_lock<std::shared_mutex> lock(data_rwlock_);
  return data_;
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline void FrameBase<DataT, Derived>::readRawData(
    std::function<void(const char*, size_t)> func) const {
  std::shared_lock<std::shared_mutex> lock(data_rwlock_);
  func(reinterpret_cast<const char*>(&data_), sizeof(DataT));
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline void FrameBase<DataT, Derived>::writeRawData(
    std::function<void(char*, size_t)> func) {
  std::unique_lock<std::shared_mutex> lock(data_rwlock_);
  func(reinterpret_cast<char*>(&data_), sizeof(DataT));
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline const char* FrameBase<DataT, Derived>::rawData() const {
  std::shared_lock<std::shared_mutex> lock(data_rwlock_);
  return reinterpret_cast<const char*>(&data_);
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline char* FrameBase<DataT, Derived>::rawData() {
  std::unique_lock<std::shared_mutex> lock(data_rwlock_);
  return reinterpret_cast<char*>(&data_);
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline size_t FrameBase<DataT, Derived>::rawDataSize() const {
  return sizeof(DataT);
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline size_t FrameBase<DataT, Derived>::size() const {
  return sizeof(DataT);
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline void FrameBase<DataT, Derived>::setSerializer(
    std::function<std::vector<uint8_t>(const Data&)> s) {
  serializer_ = std::move(s);
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline void FrameBase<DataT, Derived>::setDeserializer(
    std::function<void(Data&, const std::vector<uint8_t>&)> d) {
  deserializer_ = std::move(d);
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline std::vector<uint8_t> FrameBase<DataT, Derived>::serialize() const {
  std::shared_lock<std::shared_mutex> lock(data_rwlock_);
  return serializer_(data_);
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline bool FrameBase<DataT, Derived>::deserializeWithPublish(
    const std::vector<uint8_t>& raw) {
  {
    std::unique_lock<std::shared_mutex> lock(data_rwlock_);
    deserializer_(data_, raw);
  }
  this->notifyCallbacks();
  return raw.size() == sizeof(DataT);
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline void FrameBase<DataT, Derived>::deserialize(
    const std::vector<uint8_t>& raw) {
  std::unique_lock<std::shared_mutex> lock(data_rwlock_);
  deserializer_(data_, raw);
}

template <typename DataT, typename Derived>
  requires TriviallyCopyable<DataT>
inline std::string FrameBase<DataT, Derived>::id() const {
  return instanceName_;
}

#endif  // NEXUM_EXTERNAL_INTERFACE_FRAMEBASE_HPP