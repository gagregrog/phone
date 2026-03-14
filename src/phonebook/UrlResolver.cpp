#include "phonebook/UrlResolver.h"

// Find the host start/end positions in a URL.
// Returns false if URL is malformed (no "://").
// hostStart: index of first char of host
// hostEnd: index of first char after host (before port colon, slash, or end)
static bool findHost(const std::string& url, size_t& hostStart, size_t& hostEnd) {
    size_t schemeEnd = url.find("://");
    if (schemeEnd == std::string::npos) return false;
    hostStart = schemeEnd + 3;
    if (hostStart >= url.size()) return false;

    // Host ends at ':', '/', or end of string
    hostEnd = hostStart;
    while (hostEnd < url.size() && url[hostEnd] != ':' && url[hostEnd] != '/') {
        hostEnd++;
    }
    return hostEnd > hostStart;
}

std::string urlExtractHost(const std::string& url) {
    size_t start, end;
    if (!findHost(url, start, end)) return "";
    return url.substr(start, end - start);
}

std::string urlReplaceHost(const std::string& url, const std::string& newHost) {
    size_t start, end;
    if (!findHost(url, start, end)) return url;
    return url.substr(0, start) + newHost + url.substr(end);
}

std::string urlResolveLocal(const std::string& url,
                            std::function<std::string(const std::string&)> lookupFn) {
    std::string host = urlExtractHost(url);
    if (host.empty()) return url;

    // Check for .local suffix
    const std::string suffix = ".local";
    if (host.size() <= suffix.size()) return url;
    if (host.compare(host.size() - suffix.size(), suffix.size(), suffix) != 0) return url;

    // Strip ".local" and look up
    std::string mdnsName = host.substr(0, host.size() - suffix.size());
    std::string ip = lookupFn(mdnsName);
    if (ip.empty()) return url;

    return urlReplaceHost(url, ip);
}
