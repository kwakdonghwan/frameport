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

#ifndef NEXUM_COM_EXTERNAL_FRAME_IFRAME_H
#define NEXUM_COM_EXTERNAL_FRAME_IFRAME_H

#include <any>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../method/IMethod.h"

/**
 * @brief 콜백 실행 정책(enum)
 * - Direct: 즉시 호출
 * - Threaded: 별도 스레드에서 호출
 */
enum class CallbackPolicy { Direct, Threaded };

/**
 * @brief IFrame 인터페이스
 *
 * 신호 데이터와 콜백 등록/실행 등 프레임 구조를 관리하는 추상 클래스
 */
class IFrame : public IMethod {
 public:
  /**
   * @brief 콜백 함수 타입
   */
  using Callback = std::function<void(const IFrame&)>;
  using SnapshotCallback =
      std::function<void(const std::vector<uint8_t>&, size_t)>;
  /**
   * @brief Getter 함수 타입 (std::any 반환)
   */
  using Getter = std::function<std::any()>;
  /**
   * @brief Setter 함수 타입 (std::any 입력)
   */
  using Setter = std::function<void(const std::any&)>;
  /**
   * @brief 콜백 ID 타입
   */
  using CallbackId = uint64_t;

  /**
   * @brief 콜백 엔트리 구조체 (콜백 등록/관리)
   */
  struct CallbackEntry {
    CallbackId id;                               ///< 콜백 고유 ID
    Callback cb;                                 ///< Direct용 콜백 함수
    SnapshotCallback snapshotCb;                 ///< Threaded용 콜백 함수
    CallbackPolicy policy;                       ///< 콜백 실행 정책
    struct ThreadedData;                         ///< Threaded 정책 시 사용
    std::unique_ptr<ThreadedData> threadedData;  ///< Threaded 정책 데이터
    void stopAndJoin();
  };

  /**
   * @brief 생성자
   */
  IFrame();
  /**
   * @brief 소멸자
   */
  virtual ~IFrame();

  /**
   * @brief 모든 Threaded 콜백 스레드를 중지 및 join
   */
  void stopThreadedCallbacks();

  /**
   * @brief 프레임 고유 ID 반환 (구현 필요)
   * @return 프레임 식별자 문자열
   */
  virtual std::string id() const = 0;
  /**
   * @brief 프레임 데이터 크기 반환 (구현 필요)
   * @return 데이터 크기 (바이트)
   */
  virtual size_t size() const = 0;

  /**
   * @brief 신호 등록 (Get/Set 람다 등록)
   * @tparam T 데이터 구조체 타입
   * @tparam Field 멤버 변수 타입
   * @param name 신호명
   * @param member 멤버 포인터
   * @param data_ptr 데이터 객체 포인터
   * @param rwlock 읽기/쓰기 락 포인터
   */
  template <typename T, typename Field>
  void registerSignal(const std::string& name, Field T::* member, T* data_ptr,
                      std::shared_mutex* rwlock);

  /**
   * @brief 신호값 반환 (std::any)
   * @param name 신호명
   * @return 신호값(std::any)
   */
  virtual std::any getSignal(const std::string& name) const;
  /**
   * @brief 신호값 설정 및 콜백 알림
   * @param name 신호명
   * @param value 값
   */
  virtual void setSignalWithPublish(const std::string& name,
                                    const std::any& value);
  /**
   * @brief 신호값 설정 (콜백 미호출)
   * @param name 신호명
   * @param value 값
   */
  virtual void setSignal(const std::string& name, const std::any& value);

  /**
   * @brief 콜백 등록
   * @param cb 콜백 함수
   * @param policy 실행 정책 (기본 Threaded)
   * @return 콜백 ID
   */
  CallbackId addCallback(Callback cb,
                         CallbackPolicy policy = CallbackPolicy::Threaded);
  CallbackId addSnapshotCallback(SnapshotCallback cb);
  /**
   * @brief 콜백 해제
   * @param id 콜백 ID
   */
  void removeCallback(CallbackId id);

  /**
   * @brief 콜백 전체 실행 (notify)
   */
  void notifyCallbacks();

  /**
   * @brief 람다 기반 원시 데이터 안전 접근
   */
  virtual void readRawData(
      std::function<void(const char*, size_t)> func) const = 0;

  /**
   * @brief 람다 기반 원시 데이터 안전 접근
   */
  virtual void writeRawData(std::function<void(char*, size_t)> func) = 0;

  /**
   * @brief 바이트 배열 역직렬화 후 콜백 알림 (구현 필요)
   * @param raw 직렬화 데이터
   * @return 성공 여부
   */
  virtual bool deserializeWithPublish(const std::vector<uint8_t>& raw) = 0;
  /**
   * @brief 직렬화 (구현 필요)
   * @return 직렬화된 데이터
   */
  virtual std::vector<uint8_t> serialize() const = 0;
  /**
   * @brief 역직렬화 (구현 필요)
   * @param raw 직렬화 데이터
   */
  virtual void deserialize(const std::vector<uint8_t>& raw) = 0;

 protected:
  std::unordered_map<std::string, Getter> getters_;  ///< Getter 함수 맵
  std::unordered_map<std::string, Setter> setters_;  ///< Setter 함수 맵
  std::vector<CallbackEntry> callbacks_;             ///< 콜백 리스트
  std::atomic<CallbackId> nextCallbackId_;           ///< 다음 콜백 ID
  std::mutex cb_mutex_;                              ///< 콜백 락

  /**
   * @brief 원시 데이터 포인터 반환 (const) (구현 필요)
   * @note 매우 위험하므로 프레임워크 개발자만 사용
   * @return 데이터 포인터
   */
  virtual const char* rawData() const { return nullptr; };
  /**
   * @brief 원시 데이터 포인터 반환 (구현 필요)
   * @note 매우 위험하므로 프레임워크 개발자만 사용
   * @return 데이터 포인터
   */
  virtual char* rawData() { return nullptr; };

  virtual size_t rawDataSize() const { return 0; }  // 크기도 함께
};

/**
 * @brief Threaded 콜백 스레드 데이터 구조체
 */
struct IFrame::CallbackEntry::ThreadedData {
  std::thread worker;
  std::queue<std::vector<uint8_t>> queue;
  std::mutex mtx;
  std::condition_variable cv;
  std::atomic<bool> stop{false};
};

// ------------------- IFrame 구현부 -------------------

inline void IFrame::CallbackEntry::stopAndJoin() {
  if (policy == CallbackPolicy::Threaded && threadedData) {
    {
      std::lock_guard<std::mutex> lock(threadedData->mtx);
      threadedData->stop = true;
      threadedData->cv.notify_all();
    }
    if (threadedData->worker.joinable()) threadedData->worker.join();
  }
}

inline IFrame::IFrame() : nextCallbackId_(1) {}

inline IFrame::~IFrame() { stopThreadedCallbacks(); }

inline void IFrame::stopThreadedCallbacks() {
  std::unique_lock<std::mutex> lock(cb_mutex_);
  for (auto& entry : callbacks_) {
    entry.stopAndJoin();
  }
}

template <typename T, typename Field>
inline void IFrame::registerSignal(const std::string& name, Field T::* member,
                                   T* data_ptr, std::shared_mutex* rwlock) {
  getters_[name] = [data_ptr, member, rwlock]() -> std::any {
    std::shared_lock<std::shared_mutex> lock(*rwlock);
    return data_ptr->*member;
  };
  setters_[name] = [data_ptr, member, rwlock](const std::any& v) {
    std::unique_lock<std::shared_mutex> lock(*rwlock);
    data_ptr->*member = std::any_cast<Field>(v);
  };
}

inline std::any IFrame::getSignal(const std::string& name) const {
  auto it = getters_.find(name);
  if (it == getters_.end()) throw std::runtime_error("Unknown signal: " + name);
  return it->second();
}

inline void IFrame::setSignalWithPublish(const std::string& name,
                                         const std::any& value) {
  auto it = setters_.find(name);
  if (it == setters_.end()) throw std::runtime_error("Unknown signal: " + name);
  it->second(value);
  notifyCallbacks();
}

inline void IFrame::setSignal(const std::string& name, const std::any& value) {
  auto it = setters_.find(name);
  if (it == setters_.end()) throw std::runtime_error("Unknown signal: " + name);
  it->second(value);
}

/**
 * @brief Direct 모드 콜백 등록
 */
inline IFrame::CallbackId IFrame::addCallback(Callback cb,
                                              CallbackPolicy policy) {
  CallbackId id = nextCallbackId_.fetch_add(1);
  std::unique_lock<std::mutex> lock(cb_mutex_);
  if (policy == CallbackPolicy::Threaded) {
    throw std::logic_error("Use addSnapshotCallback for Threaded policy");
  }
  callbacks_.push_back({id, std::move(cb), nullptr, policy, nullptr});
  return id;
}

/**
 * @brief Threaded 모드 콜백 등록 (스냅샷 기반)
 */
inline IFrame::CallbackId IFrame::addSnapshotCallback(SnapshotCallback cb) {
  CallbackId id = nextCallbackId_.fetch_add(1);
  std::unique_lock<std::mutex> lock(cb_mutex_);

  auto threaded = std::make_unique<CallbackEntry::ThreadedData>();
  // worker: 큐에서 복사본 꺼내 콜백에 전달
  threaded->worker = std::thread([threadedPtr = threaded.get(), cb, id]() {
    while (true) {
      std::vector<uint8_t> snapshot;
      {
        std::unique_lock<std::mutex> lock(threadedPtr->mtx);
        threadedPtr->cv.wait(lock, [&] {
          return threadedPtr->stop || !threadedPtr->queue.empty();
        });
        if (threadedPtr->stop && threadedPtr->queue.empty()) break;
        if (!threadedPtr->queue.empty()) {
          snapshot = std::move(threadedPtr->queue.front());
          threadedPtr->queue.pop();
        }
      }
      if (!snapshot.empty()) {
        cb(snapshot, snapshot.size());
      }
    }
  });
  callbacks_.push_back({id, nullptr, std::move(cb), CallbackPolicy::Threaded,
                        std::move(threaded)});
  return id;
}

/**
 * @brief 콜백 전체 실행 (notify)
 */
inline void IFrame::notifyCallbacks() {
  std::unique_lock<std::mutex> lock(cb_mutex_);
  for (auto& entry : callbacks_) {
    if (entry.policy == CallbackPolicy::Direct && entry.cb) {
      entry.cb(*this);  // 원본 객체
    } else if (entry.policy == CallbackPolicy::Threaded && entry.snapshotCb &&
               entry.threadedData) {
      // 콜백 호출 시점의 snapshot을 큐에 push
      std::vector<uint8_t> snapshot = this->serialize();
      {
        std::lock_guard<std::mutex> qlock(entry.threadedData->mtx);
        entry.threadedData->queue.push(std::move(snapshot));
        entry.threadedData->cv.notify_one();
      }
    }
  }
}

inline void IFrame::removeCallback(CallbackId id) {
  std::unique_lock<std::mutex> lock(cb_mutex_);
  for (auto it = callbacks_.begin(); it != callbacks_.end(); ++it) {
    if (it->id == id) {
      it->stopAndJoin();
      callbacks_.erase(it);
      break;
    }
  }
}

#endif  // NEXUM_COM_EXTERNAL_FRAME_IFRAME_H
