idf_component_register(SRCS "main.c" "wifi_helper.c" "ws_helper.c" "mqtt_helper.c" "./helpers/nvs_helper.c" "espnow_helper.c" "./helpers/mem_helper.c"
                       INCLUDE_DIRS "."
                       PRIV_INCLUDE_DIRS "./json" "./helpers"
                       EMBED_FILES "front/index.html" "front/favicon.ico")
