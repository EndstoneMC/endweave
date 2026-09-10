#pragma once

namespace endweave {

class Session;

/** What a transform works with: the session, the packet's cancel flag, and the object to fill.
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

    /** The object this transform fills. */
    [[nodiscard]] To &out() const noexcept
    {
        return out_;
    }

    /** A context for a nested field, sharing this one's session and cancel flag. */
    template <class Child>
    [[nodiscard]] Context<Child> with(Child &out) const noexcept
    {
        return Context<Child>{session_, cancelled_, out};
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
    Session &session_;
    bool &cancelled_;
    To &out_;
};

} // namespace endweave
