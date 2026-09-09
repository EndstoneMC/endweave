#pragma once

namespace endweave {

class Session;

/** The connection every transform on one packet shares, the cancel any of them may answer
 * with, and the object this one fills in.
 * @see ViaVersion PacketWrapper.
 */
template <class To>
class Context {
public:
    Context(Session &session, bool &cancelled, To &out) noexcept : session_(session), cancelled_(cancelled), out_(out)
    {
    }

    /** @see ViaVersion PacketWrapper#user. */
    [[nodiscard]] Session &session() const noexcept
    {
        return session_;
    }

    /** What this transform writes into, in place of returning it. */
    [[nodiscard]] To &out() const noexcept
    {
        return out_;
    }

    /** The context a nested field's transform writes through, over this one's connection and
     * cancel. */
    template <class Child>
    [[nodiscard]] Context<Child> with(Child &out) const noexcept
    {
        return Context<Child>{session_, cancelled_, out};
    }

    /** Drops the packet rather than translating it, for a source the destination cannot
     * express. Return straight after; nothing written into the destination is read.
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
    Session &session_;
    bool &cancelled_;
    To &out_;
};

} // namespace endweave
