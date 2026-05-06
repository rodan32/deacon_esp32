#ifndef SECRETS_H
#define SECRETS_H

struct WiFiCredential {
  const char *ssid;
  const char *password;
};

// Add as many networks as you like. The device tries them in order: if one
// does not connect within ~20 seconds, it moves to the next, then repeats
// on the next hourly sync.
const WiFiCredential WIFI_NETWORKS[] = {
    {"YOUR_PRIMARY_WIFI", "YOUR_PRIMARY_PASSWORD"},
    {"YOUR_SECONDARY_WIFI", "YOUR_SECONDARY_PASSWORD"},
};

const int WIFI_NETWORKS_COUNT =
    sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);

// Put the URL to your homelab-hosted JSON file here
#define DATA_JSON_URL                                                          \
  "https://dashboard.zarchstuff.com/exports/deacon_data.json"

#endif
