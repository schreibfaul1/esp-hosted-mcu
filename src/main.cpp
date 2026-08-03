
/* Program to update the FW of the WiFI chip ESP32C6 on an ESP32-P4 board

❗❗❗  Preparation: If the ESP32-C6 is not connected as defined in "pins_arduino.h",
                     the pins must be changed in platformio.ini.

1) Select and download the appropriate FW version from: https://esphome.github.io/esp-hosted-firmware/manifest/esp32c6.json

2) Paste the downloaded file into the "/data" folder

3) Build + Upload

4) goto PIOARDUINO: "Upload Filesystem Image"

5) Reset

to erase everything close the serial terminal and go to PIOARDUINO: "Erase Flash"

*/

#include "Arduino.h"
#include "FS.h"
#include "LittleFS.h"
#include "dirent.h"
#include "pins_arduino.h"
#include "esp_app_desc.h"
#include "esp_hosted.h"
#include "esp_hosted_api_types.h"
#include "esp_hosted_ota.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "ota_littlefs/ota_littlefs.h"

esp_hosted_coprocessor_fwver_t host_version = {0}, slave_version = {0};
esp_err_t ret = ESP_OK;



// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void printLittleFSContents()
{
    DIR *dir = opendir("/littlefs");
    if (dir == nullptr)
    {
        printf("can't open /littlefs.\n");
        return;
    }

    printf("content of /littlefs:\n");

    struct dirent *entry;
    bool foundFile = false;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        foundFile = true;
        String path = String("/") + entry->d_name;
        File file = LittleFS.open(path, "r");
        if (!file)
        {
            printf("  %s (could not be opened)\n", path.c_str());
            continue;
        }

        printf("  %s - %u Bytes\n", path.c_str(), static_cast<unsigned>(file.size()));
        file.close();
    }

    if (!foundFile)
    {
        printf("  No files found.");
    }

    closedir(dir);
}
// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
int compare_versions()
{

    host_version.major1 = ESP_HOSTED_VERSION_MAJOR_1;
    host_version.minor1 = ESP_HOSTED_VERSION_MINOR_1;
    host_version.patch1 = ESP_HOSTED_VERSION_PATCH_1;

    ret = esp_hosted_get_coprocessor_fwversion(&slave_version);
    if (ret != ESP_OK)
    {
        printf("ret: %s\n", esp_err_to_name(ret));
    }

    uint32_t slave_ver = ESP_HOSTED_VERSION_VAL(slave_version.major1, slave_version.minor1, slave_version.patch1);
    uint32_t host_ver = ESP_HOSTED_VERSION_VAL(host_version.major1, host_version.minor1, host_version.patch1);

    if (host_ver == slave_ver)
    {
        printf("Version match: Host [%u.%u.%u] > Co-proc [%u.%u.%u] perfect\n", ESP_HOSTED_VERSION_PRINTF_ARGS(host_ver), ESP_HOSTED_VERSION_PRINTF_ARGS(slave_ver));
        return 0; // Versions match
    }
    else if (host_ver > slave_ver)
    {
        printf("Version mismatch: Host [%u.%u.%u] > Co-proc [%u.%u.%u] ==> Upgrade co-proc\n", ESP_HOSTED_VERSION_PRINTF_ARGS(host_ver), ESP_HOSTED_VERSION_PRINTF_ARGS(slave_ver));
        return -1; // Host newer, slave needs upgrade
    }
    else
    {
        printf("Version mismatch: Host [%u.%u.%u] < Co-proc [%u.%u.%u] ==> Upgrade host\n", ESP_HOSTED_VERSION_PRINTF_ARGS(host_ver), ESP_HOSTED_VERSION_PRINTF_ARGS(slave_ver));
        return 1; // Slave newer, host needs upgrade
    }
}
// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
int perform_slave_ota()
{
    uint8_t delete_after_flash = 0;
    printf("Starting OTA via LittleFS\n");
#ifdef CONFIG_OTA_DELETE_FILE_AFTER_FLASH
    delete_after_flash = 1;
#endif
    return ota_littlefs_perform(delete_after_flash);
}
// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
static void activate_and_restart_slave()
{
    bool activate_supported = false;
    esp_hosted_coprocessor_fwver_t current_slave_version = {0};

    if (esp_hosted_get_coprocessor_fwversion(&current_slave_version) == ESP_OK)
    {
        printf("Slave firmware before activation: %u.%u.%u\n",
                      current_slave_version.major1,
                      current_slave_version.minor1,
                      current_slave_version.patch1);

        if ((current_slave_version.major1 > 2) ||
            (current_slave_version.major1 == 2 && current_slave_version.minor1 > 5))
        {
            activate_supported = true;
        }
    }
    else
    {
        printf("Could not detect slave version before activation\n");
    }

    if (activate_supported)
    {
        esp_err_t activate_ret = esp_hosted_slave_ota_activate();
        if (activate_ret == ESP_OK)
        {
            printf("New slave firmware activated - slave will reboot\n");
        }
        else
        {
            printf("Failed to activate slave firmware: %s\n", esp_err_to_name(activate_ret));
        }
    }
    else
    {
        printf("Activate API not supported by current slave firmware\n");
    }

    printf("Restarting host to resync with slave...\n");
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
}
// —————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————————
void setup()
{
    // Serial.begin(115200);
    vTaskDelay(1000);
    printf("\n\n");
    printf("----------------------------------\n");
    printf("ESP32 Chip: %s\n", ESP.getChipModel());
    printf("Arduino Version: %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
    printf("ESP-IDF Version: %d.%d.%d\n", ESP_IDF_VERSION_MAJOR, ESP_IDF_VERSION_MINOR, ESP_IDF_VERSION_PATCH);
    printf("ARDUINO_LOOP_STACK_SIZE %d words (32 bit)\n", CONFIG_ARDUINO_LOOP_STACK_SIZE);
    printf("----------------------------------");
    printf("\n\n");

    // ... Ausgaben ...

    const esp_partition_t *p = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_LITTLEFS,
        "storage");

    if (p)
    {
        printf("LittleFS partition: addr=0x%lx size=0x%lx label=%s\n", p->address, p->size, p->label);
    }
    else
    {
        printf("LittleFS partition 'storage' not found\n");
    }

    // LittleFS mounten und offen lassen!
    if (!LittleFS.begin(false, "/littlefs", 10, "storage"))
    {
        printf("Error: Can't start littlefs\n");
        return;
    }
    printLittleFSContents();
    // LittleFS.end();  // <-- NICHT HIER! Erst nach dem OTA

    // ESP-Hosted initialisieren
    printf("Initializing ESP-Hosted...\n");
    if (nvs_flash_init() != ESP_OK)
        printf("nvs_flash_init() failed!\n");
    if (esp_event_loop_create_default() != ESP_OK)
        printf("esp_event_loop_create_default() failed!\n");
    if (esp_hosted_init() != ESP_OK)
        printf("esp_hosted_init() failed!\n");
    if (esp_hosted_connect_to_slave() != ESP_OK)
        printf("esp_hosted_connect_to_slave() failed!\n");
    printf("ESP-Hosted initialized successfully\n");

    // Version prüfen
    if (compare_versions() >= 0)
    {
        printf("Versions compatible - OTA not required\n");
        LittleFS.end(); // Erst hier enden, wenn kein OTA nötig
        return;
    }

    // OTA durchführen
    printf("Starting slave OTA update...\n");
    ret = perform_slave_ota();
    if (ret == ESP_HOSTED_SLAVE_OTA_COMPLETED)
    {
        printf("OTA completed successfully!\n");
        LittleFS.end();
        activate_and_restart_slave();
    }
    else if (ret == ESP_HOSTED_SLAVE_OTA_NOT_REQUIRED)
    {
        printf("OTA not required - slave firmware is up to date\n");
    }
    else
    {
        printf("OTA failed with status %d\n\n", ret);
    }

    LittleFS.end(); // Nach dem OTA erst enden
}

void loop()
{
    vTaskDelay(1);
}
