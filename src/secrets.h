#ifndef SECRETS_H
#define SECRETS_H

struct WiFiCredential {
  const char *ssid;
  const char *password;
};

// Replace with your actual WiFi credentials
const WiFiCredential WIFI_NETWORKS[] = {{"COCHRAN1", "8014895086"},
                                        {"Zentar", "8012254434"},
                                        {"Liahona", "alma3738"}};

const int WIFI_NETWORKS_COUNT =
    sizeof(WIFI_NETWORKS) / sizeof(WIFI_NETWORKS[0]);

// Put the URL to your homelab-hosted JSON file here
#define DATA_JSON_URL "http://YOUR_HOMELAB_IP/deacon_data.json"

#endif
