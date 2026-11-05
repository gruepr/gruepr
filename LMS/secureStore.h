#ifndef SECURESTORE_H
#define SECURESTORE_H

#include <QByteArray>
#include <QString>

// Saves small secrets (login tokens) in the operating system's own credential store: Windows Credential
// Manager, macOS Keychain, or the Secret Service keyring (via libsecret) on Linux. If the store is
// unavailable, write() returns false and the secret is NOT saved anywhere else -- the caller can still
// use it for the current session, but the user will need to log in again next time.
namespace SecureStore {
    bool write(const QString &key, const QByteArray &secret);
    QByteArray read(const QString &key);    // empty if not found or if the store is unavailable
}

#endif // SECURESTORE_H
