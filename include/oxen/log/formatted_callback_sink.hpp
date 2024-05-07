#include <concepts>
#include <functional>

#include <spdlog/sinks/base_sink.h>

#include "level.hpp"

namespace oxen::log {

using formatted_callback_t =
        std::function<void(std::string_view msg, std::string_view log_cat, Level level)>;

/// Callback sink that invokes a callback with the log message formatted according to the sink's
/// formatter.  This is meant as a less raw version of spdlog::sinks::callback_sink_mt which passes
/// along the raw details but completely ignores any log statement formatting.
///
/// This can be constructed in either of two versions: a simple version that just takes the
/// formatted string; or a more informative callback that also takes the category and level.
class formatted_callback_sink : public spdlog::sinks::base_sink<std::mutex> {
  private:
    formatted_callback_t callback_;

  public:
    // Takes a callback of the full signature:
    explicit formatted_callback_sink(formatted_callback_t callback) :
            callback_{std::move(callback)} {}

    // Takes a callable taking just a string_view:
    template <typename Callback>
        requires std::regular_invocable<Callback, std::string_view>
    explicit formatted_callback_sink(Callback&& callback) :
            formatted_callback_sink{
                    [cb = std::forward<Callback>(callback)](
                            std::string_view msg, std::string_view, Level) { cb(msg); }} {}

  protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t buf;
        formatter_->format(msg, buf);
        callback_(
                {buf.data(), buf.size()},
                {msg.logger_name.data(), msg.logger_name.size()},
                msg.level);
    }

    void flush_() override {}
};

}  // namespace oxen::log
