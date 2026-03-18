#ifndef SECRETS_H
#define SECRETS_H

struct WiFiCredential {
  const char *ssid;
  const char *password;
};

// Replace with your actual WiFi credentials
const WiFiCredential WIFI_NETWORKS[] = {
    {"YOUR_WIFI_NAME", "YOUR_WIFI_PASSWORD"}};

const int WIFI_NETWORKS_COUNT =
    sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);

// Put the URL to your homelab-hosted JSON file here
#define DATA_JSON_URL                                                          \
  "https://dashboard.zarchstuff.com/exports/deacon_data.json"

#endif
