#pragma once
#include <string>
#include <functional>

// Extracts the hostname from a URL. Returns empty string if URL is malformed.
// Example: "http://wled.local:80/json/state" -> "wled.local"
std::string urlExtractHost(const std::string& url);

// Replaces the hostname in a URL with a new value.
// Example: urlReplaceHost("http://wled.local:80/json/state", "192.168.1.50")
//       -> "http://192.168.1.50:80/json/state"
std::string urlReplaceHost(const std::string& url, const std::string& newHost);

// Resolves .local hostnames in a URL using the provided lookup function.
// If the hostname doesn't end in ".local" or lookup returns empty, returns
// the original URL unchanged.
// lookupFn: takes a hostname without ".local" suffix, returns IP string or "".
std::string urlResolveLocal(const std::string& url,
                            std::function<std::string(const std::string&)> lookupFn);
