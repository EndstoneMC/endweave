#pragma once

namespace endweave {

/** What a transform works with: the packet's cancel flag and the object to fill.
 * @see ViaVersion PacketWrapper.
 */
template <class To>
class Context {
public:
    Context(bool &cancelled, To &out) noexcept : cancelled_(cancelled), out_(out) {}

    /** The object this transform fills. */
    [[nodiscard]] To &out() const noexcept
    {
        return out_;
    }

    /** A context for a nested field, sharing this one's cancel flag. */
    template <class Child>
    [[nodiscard]] Context<Child> with(Child &out) const noexcept
    {
        return Context<Child>{cancelled_, out};
    }

    /** Drops the packet. Return right after; nothing written to the output is used.
     * @see ViaVersion PacketWrapper#cancel.
     */
    void cancel() const noexcept
    {
        cancelled_ = true;
    }

    /** @see ViaVersion PacketWrapper#isCancelled. */
    [[nodiscard]] bool isCancelled() const noexcept
    {
        return cancelled_;
    }

private:
    bool &cancelled_;
    To &out_;
};

} // namespace endweave
