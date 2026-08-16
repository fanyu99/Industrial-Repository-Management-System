#pragma once
#include <QtGlobal>
// 动态刷新状态
enum class OptionLoadState {
    Idle,
    Loading,
    Ready,
    Failed
};
// 刷新上下文结构体
struct OptionLoadTracker {
    quint64 requestId { 0 };
    OptionLoadState state { OptionLoadState::Idle };
};
