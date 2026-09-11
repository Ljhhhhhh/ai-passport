#!/usr/bin/env python3
"""Project scaffolding tool for FoloToy AI Passport Project Collection."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
PROJECTS_DIR = ROOT / "projects"

NAME_PATTERN = re.compile(r"^[a-z0-9][a-z0-9_-]{1,63}$")


def sanitize_title(name: str) -> str:
    parts = re.split(r"[-_]+", name)
    return " ".join(p.capitalize() for p in parts)


def create_project(name: str, title: str | None = None) -> Path:
    if not NAME_PATTERN.match(name):
        print(
            f"ERROR: Invalid project name '{name}'. Use lowercase letters, digits, hyphens, or underscores (2-64 chars).",
            file=sys.stderr,
        )
        sys.exit(1)

    project_dir = PROJECTS_DIR / name
    if project_dir.exists():
        print(
            f"ERROR: Project directory '{project_dir.relative_to(ROOT)}' already exists.",
            file=sys.stderr,
        )
        sys.exit(1)

    display_title = title or sanitize_title(name)

    # Create directory structure
    (project_dir / "main").mkdir(parents=True, exist_ok=True)
    (project_dir / "tests").mkdir(parents=True, exist_ok=True)
    (project_dir / "assets").mkdir(parents=True, exist_ok=True)

    # 1. Project CMakeLists.txt
    cmake_project = f"""cmake_minimum_required(VERSION 3.16)

# Include shared repository components (BSP, etc.)
set(EXTRA_COMPONENT_DIRS "${{CMAKE_CURRENT_LIST_DIR}}/../../components")

include($ENV{{IDF_PATH}}/tools/cmake/project.cmake)
project({name})
"""
    (project_dir / "CMakeLists.txt").write_text(cmake_project, encoding="utf-8")

    # 2. Main component CMakeLists.txt
    cmake_main = """idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES bsp nvs_flash
)
"""
    (project_dir / "main" / "CMakeLists.txt").write_text(cmake_main, encoding="utf-8")

    # 3. Main C starter file
    main_c = f"""// {name}/main/main.c - {display_title} for FoloToy AI Passport
#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_battery.h"
#include "lvgl.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "{name}";

static lv_obj_t *s_label_info = NULL;

static void button_task(void *pvParameters)
{{
    (void)pvParameters;
    bsp_button_event_t event;
    while (1) {{
        if (bsp_button_receive(&event, portMAX_DELAY)) {{
            ESP_LOGI(TAG, "Button: %d, Event: %d", event.button, event.event);
            if (bsp_lvgl_lock(pdMS_TO_TICKS(100))) {{
                if (s_label_info != NULL) {{
                    const char *btn_name = "Unknown";
                    if (event.button == BSP_BTN_UP) btn_name = "UP";
                    else if (event.button == BSP_BTN_DOWN) btn_name = "DOWN";
                    else if (event.button == BSP_BTN_OK) btn_name = "OK";

                    const char *evt_name = (event.event == BSP_BTN_PRESS) ? "Click" : "Long Press";
                    lv_label_set_text_fmt(s_label_info, "Key: %s (%s)", btn_name, evt_name);
                }}
                bsp_lvgl_unlock();
            }}
        }}
    }}
}}

void app_main(void)
{{
    ESP_LOGI(TAG, "Starting {display_title}...");

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {{
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }}
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(bsp_i2c_init());
    ESP_ERROR_CHECK(bsp_display_init());
    ESP_ERROR_CHECK(bsp_button_init());
    ESP_ERROR_CHECK(bsp_battery_init());

    if (bsp_lvgl_lock(pdMS_TO_TICKS(1000))) {{
        lv_obj_t *scr = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(scr, lv_color_hex(0x1E1E2E), 0);

        lv_obj_t *title_label = lv_label_create(scr);
        lv_label_set_text(title_label, "{display_title}");
        lv_obj_set_style_text_color(title_label, lv_color_hex(0xF5E0DC), 0);
        lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
        lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 40);

        s_label_info = lv_label_create(scr);
        lv_label_set_text(s_label_info, "Press any key to test");
        lv_obj_set_style_text_color(s_label_info, lv_color_hex(0x89B4FA), 0);
        lv_obj_set_style_text_font(s_label_info, &lv_font_montserrat_14, 0);
        lv_obj_align(s_label_info, LV_ALIGN_CENTER, 0, 10);

        lv_screen_load(scr);
        bsp_lvgl_unlock();
    }}

    xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "{display_title} initialized successfully.");
}}
"""
    (project_dir / "main" / "main.c").write_text(main_c, encoding="utf-8")

    # 4. Copy standard sdkconfig.defaults and partitions.csv
    root_sdkconfig = ROOT / "sdkconfig.defaults"
    if root_sdkconfig.is_file():
        (project_dir / "sdkconfig.defaults").write_text(
            root_sdkconfig.read_text(encoding="utf-8"), encoding="utf-8"
        )

    root_partitions = ROOT / "partitions.csv"
    if root_partitions.is_file():
        (project_dir / "partitions.csv").write_text(
            root_partitions.read_text(encoding="utf-8"), encoding="utf-8"
        )

    # 5. Sample host test
    test_sample = f"""// test_sample.c - Sample host unit test for {name}
#include <assert.h>
#include <stdio.h>

int sample_add(int a, int b)
{{
    return a + b;
}}

int main(void)
{{
    assert(sample_add(2, 3) == 5);
    assert(sample_add(-1, 1) == 0);
    printf("Sample host test passed!\\n");
    return 0;
}}
"""
    (project_dir / "tests" / "test_sample.c").write_text(test_sample, encoding="utf-8")

    # 6. Bilingual README files
    readme_en = f"""<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# {display_title}

An application for FoloToy AI Passport (ESP32-C3).

## Features

- Built with ESP-IDF and LVGL 9
- Uses shared Board Support Package (`components/bsp`)
- Hardware peripheral integration (ST7789 display, physical buttons, battery ADC)

## Building and Flashing

From the repository root:

```bash
# Build firmware
idf.py -C projects/{name} build

# Flash to device and open monitor
idf.py -C projects/{name} flash monitor
```

## Running Host Tests

```bash
cc -std=c11 -Wall -Wextra -Werror \\
    projects/{name}/tests/test_sample.c \\
    -o /tmp/test_{name}_sample && /tmp/test_{name}_sample
```
"""
    (project_dir / "README.md").write_text(readme_en, encoding="utf-8")

    readme_zh = f"""<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# {display_title}

专为 FoloToy AI Passport (ESP32-C3) 打造的应用程序。

## 功能特性

- 基于 ESP-IDF 与 LVGL 9 开发
- 使用公共板级支持包（`components/bsp`）
- 硬件外设集成（ST7789 彩屏、实体按键、电池 ADC 检测）

## 编译与烧录

在仓库根目录下运行：

```bash
# 编译固件
idf.py -C projects/{name} build

# 烧录到设备并打开串口监视器
idf.py -C projects/{name} flash monitor
```

## 运行主机测试

```bash
cc -std=c11 -Wall -Wextra -Werror \\
    projects/{name}/tests/test_sample.c \\
    -o /tmp/test_{name}_sample && /tmp/test_{name}_sample
```
"""
    (project_dir / "README.zh_CN.md").write_text(readme_zh, encoding="utf-8")

    print(f"SUCCESS: Project created at projects/{name}")
    print("\nNext steps:")
    print(f"  1. Build firmware:      idf.py -C projects/{name} build")
    print(f"  2. Flash and monitor:   idf.py -C projects/{name} flash monitor")
    print("  3. Run validation:      ./tools/validate.sh --static")
    return project_dir


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Scaffold a new project in the FoloToy AI Passport Project Collection."
    )
    parser.add_argument(
        "name",
        help="Project folder name (e.g. 'pomodoro-timer', 'wooden-fish', 'weather-clock')",
    )
    parser.add_argument(
        "--title",
        "-t",
        help="Human-readable project title (e.g. 'Pomodoro Timer')",
        default=None,
    )
    args = parser.parse_args()

    create_project(args.name, args.title)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
