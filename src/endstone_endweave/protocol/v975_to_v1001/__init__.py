"""v975 <-> v1001 protocol package.

Minimal shim: no per-packet handlers required yet. Keeps newer clients (1001) connectable
to servers running 975 by providing a Protocol factory that can be registered in the
plugin's protocol manager.
"""

from .protocol import create_protocol
