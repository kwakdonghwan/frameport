
// 예시 모음
// // 신호별 접근 래퍼 (프레임 이름별 struct 내부에 static 생성)
// struct RxFrameAWrapper {
//     ISignal* signal_;
//     RxFrameAWrapper(ISignal* s) : signal_(s) {}

//     int Sig1() const { return std::any_cast<int>(signal_->get("RxFrameA",
//     "Sig1")); }
//     // ... 추가 신호
// };
// struct TxFrameAWrapper {
//     ISignal* signal_;
//     TxFrameAWrapper(ISignal* s) : signal_(s) {}

//     void txSig1(int v) { signal_->set("TxFrameA", "txSig1", v); }
//     void publish()     { signal_->publish("TxFrameA"); }
// };
// struct SignalImpl : ISignal {
//     RxFrameAWrapper RxFrameA;
//     TxFrameAWrapper TxFrameA;
//     SignalImpl() : RxFrameA(this), TxFrameA(this) {}
//     // get/set/publish 구현은 위 ISignal 상속 후 내부에 FrameBus에 위임
// };