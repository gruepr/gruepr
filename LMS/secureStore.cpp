#include "secureStore.h"

#if defined(Q_OS_WIN)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <wincred.h>
#elif defined(Q_OS_MACOS)
    #include <Security/Security.h>
#elif defined(Q_OS_LINUX)
    // glib uses "signals" as an identifier, which Qt defines as a macro
    #pragma push_macro("signals")
    #undef signals
    #include <libsecret/secret.h>
    #pragma pop_macro("signals")
    static const SecretSchema GRUEPRSCHEMA = {"com.gruepr.gruepr", SECRET_SCHEMA_NONE,
                                              {{"key", SECRET_SCHEMA_ATTRIBUTE_STRING}, {nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING}}};
#endif

bool SecureStore::write(const QString &key, const QByteArray &secret)
{
#if defined(Q_OS_WIN)
    std::wstring target = (QStringLiteral("gruepr/") + key).toStdWString();
    CREDENTIALW credential = {};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = target.data();
    credential.CredentialBlobSize = DWORD(secret.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(const_cast<char*>(secret.constData()));
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    return CredWriteW(&credential, 0) == TRUE;
#elif defined(Q_OS_MACOS)
    CFStringRef service = QStringLiteral("gruepr").toCFString();
    CFStringRef account = key.toCFString();
    CFDataRef data = secret.toCFData();
    CFMutableDictionaryRef query = CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, service);
    CFDictionarySetValue(query, kSecAttrAccount, account);
    CFMutableDictionaryRef update = CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(update, kSecValueData, data);
    OSStatus status = SecItemUpdate(query, update);
    if(status == errSecItemNotFound) {
        CFDictionarySetValue(query, kSecValueData, data);
        status = SecItemAdd(query, nullptr);
    }
    CFRelease(update);
    CFRelease(query);
    CFRelease(data);
    CFRelease(account);
    CFRelease(service);
    return status == errSecSuccess;
#elif defined(Q_OS_LINUX)
    const QByteArray keyUtf8 = key.toUtf8();
    GError *error = nullptr;
    const gboolean stored = secret_password_store_sync(&GRUEPRSCHEMA, SECRET_COLLECTION_DEFAULT, ("gruepr " + keyUtf8).constData(),
                                                       secret.toBase64().constData(), nullptr, &error, "key", keyUtf8.constData(), nullptr);
    if(error != nullptr) {
        g_error_free(error);
    }
    return stored == TRUE;
#else
    Q_UNUSED(key)
    Q_UNUSED(secret)
    return false;
#endif
}

QByteArray SecureStore::read(const QString &key)
{
    QByteArray secret;
#if defined(Q_OS_WIN)
    const std::wstring target = (QStringLiteral("gruepr/") + key).toStdWString();
    PCREDENTIALW credential = nullptr;
    if(CredReadW(target.c_str(), CRED_TYPE_GENERIC, 0, &credential) == TRUE) {
        secret = QByteArray(reinterpret_cast<const char*>(credential->CredentialBlob), int(credential->CredentialBlobSize));
        CredFree(credential);
    }
#elif defined(Q_OS_MACOS)
    CFStringRef service = QStringLiteral("gruepr").toCFString();
    CFStringRef account = key.toCFString();
    CFMutableDictionaryRef query = CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(query, kSecClass, kSecClassGenericPassword);
    CFDictionarySetValue(query, kSecAttrService, service);
    CFDictionarySetValue(query, kSecAttrAccount, account);
    CFDictionarySetValue(query, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitOne);
    CFTypeRef result = nullptr;
    if(SecItemCopyMatching(query, &result) == errSecSuccess && result != nullptr) {
        secret = QByteArray::fromCFData(static_cast<CFDataRef>(result));
        CFRelease(result);
    }
    CFRelease(query);
    CFRelease(account);
    CFRelease(service);
#elif defined(Q_OS_LINUX)
    const QByteArray keyUtf8 = key.toUtf8();
    GError *error = nullptr;
    gchar *stored = secret_password_lookup_sync(&GRUEPRSCHEMA, nullptr, &error, "key", keyUtf8.constData(), nullptr);
    if(error != nullptr) {
        g_error_free(error);
    }
    if(stored != nullptr) {
        secret = QByteArray::fromBase64(stored);
        secret_password_free(stored);
    }
#else
    Q_UNUSED(key)
#endif
    return secret;
}
