#pragma once

#include <QString>
#include <QList>

/// Represents a single VPN server location.
struct VpnServer {
    QString country;     ///< Display name, e.g. "United States"
    QString code;        ///< Two-letter country code, e.g. "us"
    QString flag;        ///< Unicode flag emoji
    QString configName;  ///< WireGuard config name without extension, e.g. "dkt-us"
};

/// Returns the built-in list of supported VPN server locations.
inline QList<VpnServer> defaultServers()
{
    return {
        { "United States",  "us", "\U0001F1FA\U0001F1F8", "dkt-us" },
        { "United Kingdom", "uk", "\U0001F1EC\U0001F1E7", "dkt-uk" },
        { "Germany",        "de", "\U0001F1E9\U0001F1EA", "dkt-de" },
        { "Japan",          "jp", "\U0001F1EF\U0001F1F5", "dkt-jp" },
        { "Canada",         "ca", "\U0001F1E8\U0001F1E6", "dkt-ca" },
        { "Australia",      "au", "\U0001F1E6\U0001F1FA", "dkt-au" },
        { "Brazil",         "br", "\U0001F1E7\U0001F1F7", "dkt-br" },
        { "France",         "fr", "\U0001F1EB\U0001F1F7", "dkt-fr" },
        { "Netherlands",    "nl", "\U0001F1F3\U0001F1F1", "dkt-nl" },
        { "Singapore",      "sg", "\U0001F1F8\U0001F1EC", "dkt-sg" },
    };
}
