#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <string>

class Timestamp {
public:
    Timestamp();
    explicit Timestamp(int64_t microSecondsSinceEpoch);

    void swap(Timestamp& that) {
        std::swap(microSecondsSinceEpoch_, that.microSecondsSinceEpoch_);
    }

    // 获取当前时间
    static Timestamp now();

    // 获取时间字符串表示
    std::string toString() const;
    std::string toFormattedString(bool showMicroseconds = true) const;

    // 判断是否有效
    bool valid() const { return microSecondsSinceEpoch_ > 0; }

    // 获取微秒数
    int64_t microSecondsSinceEpoch() const { return microSecondsSinceEpoch_; }

    // 静态常量
    static const int kMicroSecondsPerSecond = 1000 * 1000;

private:
    int64_t microSecondsSinceEpoch_;
};

// 比较运算符
inline bool operator<(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

// 计算时间差（秒）
inline double timeDifference(Timestamp high, Timestamp low) {
    int64_t diff = high.microSecondsSinceEpoch() - low.microSecondsSinceEpoch();
    return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

// 增加指定秒数
inline Timestamp addTime(Timestamp timestamp, double seconds) {
    int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}

#endif // TIMESTAMP_H