#include "cards_catalog.h"

extern const uint8_t s_ba_i4_start[] asm("_binary_ba_i4_start");
extern const uint8_t s_ba_ima_start[] asm("_binary_ba_ima_start");
extern const uint8_t s_ma_i4_start[] asm("_binary_ma_i4_start");
extern const uint8_t s_ma_ima_start[] asm("_binary_ma_ima_start");
extern const uint8_t s_bao_i4_start[] asm("_binary_bao_i4_start");
extern const uint8_t s_bao_ima_start[] asm("_binary_bao_ima_start");
extern const uint8_t s_ren_i4_start[] asm("_binary_ren_i4_start");
extern const uint8_t s_ren_ima_start[] asm("_binary_ren_ima_start");
extern const uint8_t s_shou_i4_start[] asm("_binary_shou_i4_start");
extern const uint8_t s_shou_ima_start[] asm("_binary_shou_ima_start");
extern const uint8_t s_yan_i4_start[] asm("_binary_yan_i4_start");
extern const uint8_t s_yan_ima_start[] asm("_binary_yan_ima_start");
extern const uint8_t s_er_i4_start[] asm("_binary_er_i4_start");
extern const uint8_t s_er_ima_start[] asm("_binary_er_ima_start");
extern const uint8_t s_tou_i4_start[] asm("_binary_tou_i4_start");
extern const uint8_t s_tou_ima_start[] asm("_binary_tou_ima_start");
extern const uint8_t s_jiao_i4_start[] asm("_binary_jiao_i4_start");
extern const uint8_t s_jiao_ima_start[] asm("_binary_jiao_ima_start");
extern const uint8_t s_ya_i4_start[] asm("_binary_ya_i4_start");
extern const uint8_t s_ya_ima_start[] asm("_binary_ya_ima_start");
extern const uint8_t s_men_i4_start[] asm("_binary_men_i4_start");
extern const uint8_t s_men_ima_start[] asm("_binary_men_ima_start");
extern const uint8_t s_deng_i4_start[] asm("_binary_deng_i4_start");
extern const uint8_t s_deng_ima_start[] asm("_binary_deng_ima_start");
extern const uint8_t s_chuang_i4_start[] asm("_binary_chuang_i4_start");
extern const uint8_t s_chuang_ima_start[] asm("_binary_chuang_ima_start");
extern const uint8_t s_bei_i4_start[] asm("_binary_bei_i4_start");
extern const uint8_t s_bei_ima_start[] asm("_binary_bei_ima_start");
extern const uint8_t s_shu_i4_start[] asm("_binary_shu_i4_start");
extern const uint8_t s_shu_ima_start[] asm("_binary_shu_ima_start");
extern const uint8_t s_che_i4_start[] asm("_binary_che_i4_start");
extern const uint8_t s_che_ima_start[] asm("_binary_che_ima_start");
extern const uint8_t s_qiu_i4_start[] asm("_binary_qiu_i4_start");
extern const uint8_t s_qiu_ima_start[] asm("_binary_qiu_ima_start");
extern const uint8_t s_xie_i4_start[] asm("_binary_xie_i4_start");
extern const uint8_t s_xie_ima_start[] asm("_binary_xie_ima_start");
extern const uint8_t s_fan_i4_start[] asm("_binary_fan_i4_start");
extern const uint8_t s_fan_ima_start[] asm("_binary_fan_ima_start");
extern const uint8_t s_nai_i4_start[] asm("_binary_nai_i4_start");
extern const uint8_t s_nai_ima_start[] asm("_binary_nai_ima_start");
extern const uint8_t s_mao_i4_start[] asm("_binary_mao_i4_start");
extern const uint8_t s_mao_ima_start[] asm("_binary_mao_ima_start");
extern const uint8_t s_gou_i4_start[] asm("_binary_gou_i4_start");
extern const uint8_t s_gou_ima_start[] asm("_binary_gou_ima_start");
extern const uint8_t s_yu_i4_start[] asm("_binary_yu_i4_start");
extern const uint8_t s_yu_ima_start[] asm("_binary_yu_ima_start");
extern const uint8_t s_niao_i4_start[] asm("_binary_niao_i4_start");
extern const uint8_t s_niao_ima_start[] asm("_binary_niao_ima_start");
extern const uint8_t s_chong_i4_start[] asm("_binary_chong_i4_start");
extern const uint8_t s_chong_ima_start[] asm("_binary_chong_ima_start");
extern const uint8_t s_ma_animal_i4_start[] asm("_binary_ma_animal_i4_start");
extern const uint8_t s_ma_animal_ima_start[] asm("_binary_ma_animal_ima_start");
extern const uint8_t s_niu_i4_start[] asm("_binary_niu_i4_start");
extern const uint8_t s_niu_ima_start[] asm("_binary_niu_ima_start");
extern const uint8_t s_yang_i4_start[] asm("_binary_yang_i4_start");
extern const uint8_t s_yang_ima_start[] asm("_binary_yang_ima_start");
extern const uint8_t s_tu_i4_start[] asm("_binary_tu_i4_start");
extern const uint8_t s_tu_ima_start[] asm("_binary_tu_ima_start");
extern const uint8_t s_xiong_i4_start[] asm("_binary_xiong_i4_start");
extern const uint8_t s_xiong_ima_start[] asm("_binary_xiong_ima_start");
extern const uint8_t s_shan_i4_start[] asm("_binary_shan_i4_start");
extern const uint8_t s_shan_ima_start[] asm("_binary_shan_ima_start");
extern const uint8_t s_shui_i4_start[] asm("_binary_shui_i4_start");
extern const uint8_t s_shui_ima_start[] asm("_binary_shui_ima_start");
extern const uint8_t s_tian_i4_start[] asm("_binary_tian_i4_start");
extern const uint8_t s_tian_ima_start[] asm("_binary_tian_ima_start");
extern const uint8_t s_yue_i4_start[] asm("_binary_yue_i4_start");
extern const uint8_t s_yue_ima_start[] asm("_binary_yue_ima_start");
extern const uint8_t s_yu_rain_i4_start[] asm("_binary_yu_rain_i4_start");
extern const uint8_t s_yu_rain_ima_start[] asm("_binary_yu_rain_ima_start");
extern const uint8_t s_hua_i4_start[] asm("_binary_hua_i4_start");
extern const uint8_t s_hua_ima_start[] asm("_binary_hua_ima_start");
extern const uint8_t s_shu_tree_i4_start[] asm("_binary_shu_tree_i4_start");
extern const uint8_t s_shu_tree_ima_start[] asm("_binary_shu_tree_ima_start");
extern const uint8_t s_hong_i4_start[] asm("_binary_hong_i4_start");
extern const uint8_t s_hong_ima_start[] asm("_binary_hong_ima_start");
extern const uint8_t s_huang_i4_start[] asm("_binary_huang_i4_start");
extern const uint8_t s_huang_ima_start[] asm("_binary_huang_ima_start");
extern const uint8_t s_lan_i4_start[] asm("_binary_lan_i4_start");
extern const uint8_t s_lan_ima_start[] asm("_binary_lan_ima_start");
extern const uint8_t s_da_i4_start[] asm("_binary_da_i4_start");
extern const uint8_t s_da_ima_start[] asm("_binary_da_ima_start");
extern const uint8_t s_xiao_i4_start[] asm("_binary_xiao_i4_start");
extern const uint8_t s_xiao_ima_start[] asm("_binary_xiao_ima_start");
extern const uint8_t s_shang_i4_start[] asm("_binary_shang_i4_start");
extern const uint8_t s_shang_ima_start[] asm("_binary_shang_ima_start");
extern const uint8_t s_xia_i4_start[] asm("_binary_xia_i4_start");
extern const uint8_t s_xia_ima_start[] asm("_binary_xia_ima_start");
extern const uint8_t s_kai_i4_start[] asm("_binary_kai_i4_start");
extern const uint8_t s_kai_ima_start[] asm("_binary_kai_ima_start");
extern const uint8_t s_guan_i4_start[] asm("_binary_guan_i4_start");
extern const uint8_t s_guan_ima_start[] asm("_binary_guan_ima_start");
extern const uint8_t s_chi_i4_start[] asm("_binary_chi_i4_start");
extern const uint8_t s_chi_ima_start[] asm("_binary_chi_ima_start");
extern const uint8_t s_he_i4_start[] asm("_binary_he_i4_start");
extern const uint8_t s_he_ima_start[] asm("_binary_he_ima_start");
extern const uint8_t s_pao_i4_start[] asm("_binary_pao_i4_start");
extern const uint8_t s_pao_ima_start[] asm("_binary_pao_ima_start");
extern const uint8_t s_tiao_i4_start[] asm("_binary_tiao_i4_start");
extern const uint8_t s_tiao_ima_start[] asm("_binary_tiao_ima_start");

const cards_entry_t cards_catalog[CARDS_COUNT] = {
    {
        .id = "ba",
        .hanzi = "爸",
        .phrase = "爸爸",
        .image = s_ba_i4_start,
        .image_len = 16464U,
        .voice = s_ba_ima_start,
        .voice_len = 7876U
    },
    {
        .id = "ma",
        .hanzi = "妈",
        .phrase = "妈妈",
        .image = s_ma_i4_start,
        .image_len = 16464U,
        .voice = s_ma_ima_start,
        .voice_len = 8481U
    },
    {
        .id = "bao",
        .hanzi = "宝",
        .phrase = "宝宝",
        .image = s_bao_i4_start,
        .image_len = 16464U,
        .voice = s_bao_ima_start,
        .voice_len = 8897U
    },
    {
        .id = "ren",
        .hanzi = "人",
        .phrase = "家人",
        .image = s_ren_i4_start,
        .image_len = 16464U,
        .voice = s_ren_ima_start,
        .voice_len = 9597U
    },
    {
        .id = "shou",
        .hanzi = "手",
        .phrase = "小手",
        .image = s_shou_i4_start,
        .image_len = 16464U,
        .voice = s_shou_ima_start,
        .voice_len = 10791U
    },
    {
        .id = "yan",
        .hanzi = "眼",
        .phrase = "眼睛",
        .image = s_yan_i4_start,
        .image_len = 16464U,
        .voice = s_yan_ima_start,
        .voice_len = 9673U
    },
    {
        .id = "er",
        .hanzi = "耳",
        .phrase = "耳朵",
        .image = s_er_i4_start,
        .image_len = 16464U,
        .voice = s_er_ima_start,
        .voice_len = 8434U
    },
    {
        .id = "tou",
        .hanzi = "头",
        .phrase = "头发",
        .image = s_tou_i4_start,
        .image_len = 16464U,
        .voice = s_tou_ima_start,
        .voice_len = 9223U
    },
    {
        .id = "jiao",
        .hanzi = "脚",
        .phrase = "小脚",
        .image = s_jiao_i4_start,
        .image_len = 16464U,
        .voice = s_jiao_ima_start,
        .voice_len = 10626U
    },
    {
        .id = "ya",
        .hanzi = "牙",
        .phrase = "牙齿",
        .image = s_ya_i4_start,
        .image_len = 16464U,
        .voice = s_ya_ima_start,
        .voice_len = 8837U
    },
    {
        .id = "men",
        .hanzi = "门",
        .phrase = "大门",
        .image = s_men_i4_start,
        .image_len = 16464U,
        .voice = s_men_ima_start,
        .voice_len = 9411U
    },
    {
        .id = "deng",
        .hanzi = "灯",
        .phrase = "开灯",
        .image = s_deng_i4_start,
        .image_len = 16464U,
        .voice = s_deng_ima_start,
        .voice_len = 9227U
    },
    {
        .id = "chuang",
        .hanzi = "床",
        .phrase = "小床",
        .image = s_chuang_i4_start,
        .image_len = 16464U,
        .voice = s_chuang_ima_start,
        .voice_len = 11954U
    },
    {
        .id = "bei",
        .hanzi = "杯",
        .phrase = "水杯",
        .image = s_bei_i4_start,
        .image_len = 16464U,
        .voice = s_bei_ima_start,
        .voice_len = 8810U
    },
    {
        .id = "shu",
        .hanzi = "书",
        .phrase = "看书",
        .image = s_shu_i4_start,
        .image_len = 16464U,
        .voice = s_shu_ima_start,
        .voice_len = 9677U
    },
    {
        .id = "che",
        .hanzi = "车",
        .phrase = "汽车",
        .image = s_che_i4_start,
        .image_len = 16464U,
        .voice = s_che_ima_start,
        .voice_len = 10096U
    },
    {
        .id = "qiu",
        .hanzi = "球",
        .phrase = "皮球",
        .image = s_qiu_i4_start,
        .image_len = 16464U,
        .voice = s_qiu_ima_start,
        .voice_len = 10533U
    },
    {
        .id = "xie",
        .hanzi = "鞋",
        .phrase = "鞋子",
        .image = s_xie_i4_start,
        .image_len = 16464U,
        .voice = s_xie_ima_start,
        .voice_len = 9118U
    },
    {
        .id = "fan",
        .hanzi = "饭",
        .phrase = "吃饭",
        .image = s_fan_i4_start,
        .image_len = 16464U,
        .voice = s_fan_ima_start,
        .voice_len = 9541U
    },
    {
        .id = "nai",
        .hanzi = "奶",
        .phrase = "牛奶",
        .image = s_nai_i4_start,
        .image_len = 16464U,
        .voice = s_nai_ima_start,
        .voice_len = 9053U
    },
    {
        .id = "mao",
        .hanzi = "猫",
        .phrase = "小猫",
        .image = s_mao_i4_start,
        .image_len = 16464U,
        .voice = s_mao_ima_start,
        .voice_len = 9967U
    },
    {
        .id = "gou",
        .hanzi = "狗",
        .phrase = "小狗",
        .image = s_gou_i4_start,
        .image_len = 16464U,
        .voice = s_gou_ima_start,
        .voice_len = 9644U
    },
    {
        .id = "yu",
        .hanzi = "鱼",
        .phrase = "小鱼",
        .image = s_yu_i4_start,
        .image_len = 16464U,
        .voice = s_yu_ima_start,
        .voice_len = 8652U
    },
    {
        .id = "niao",
        .hanzi = "鸟",
        .phrase = "小鸟",
        .image = s_niao_i4_start,
        .image_len = 16464U,
        .voice = s_niao_ima_start,
        .voice_len = 10728U
    },
    {
        .id = "chong",
        .hanzi = "虫",
        .phrase = "小虫",
        .image = s_chong_i4_start,
        .image_len = 16464U,
        .voice = s_chong_ima_start,
        .voice_len = 11490U
    },
    {
        .id = "ma_animal",
        .hanzi = "马",
        .phrase = "小马",
        .image = s_ma_animal_i4_start,
        .image_len = 16464U,
        .voice = s_ma_animal_ima_start,
        .voice_len = 9318U
    },
    {
        .id = "niu",
        .hanzi = "牛",
        .phrase = "小牛",
        .image = s_niu_i4_start,
        .image_len = 16464U,
        .voice = s_niu_ima_start,
        .voice_len = 10915U
    },
    {
        .id = "yang",
        .hanzi = "羊",
        .phrase = "小羊",
        .image = s_yang_i4_start,
        .image_len = 16464U,
        .voice = s_yang_ima_start,
        .voice_len = 10046U
    },
    {
        .id = "tu",
        .hanzi = "兔",
        .phrase = "小兔",
        .image = s_tu_i4_start,
        .image_len = 16464U,
        .voice = s_tu_ima_start,
        .voice_len = 9397U
    },
    {
        .id = "xiong",
        .hanzi = "熊",
        .phrase = "小熊",
        .image = s_xiong_i4_start,
        .image_len = 16464U,
        .voice = s_xiong_ima_start,
        .voice_len = 11069U
    },
    {
        .id = "shan",
        .hanzi = "山",
        .phrase = "大山",
        .image = s_shan_i4_start,
        .image_len = 16464U,
        .voice = s_shan_ima_start,
        .voice_len = 10605U
    },
    {
        .id = "shui",
        .hanzi = "水",
        .phrase = "喝水",
        .image = s_shui_i4_start,
        .image_len = 16464U,
        .voice = s_shui_ima_start,
        .voice_len = 10512U
    },
    {
        .id = "tian",
        .hanzi = "天",
        .phrase = "天空",
        .image = s_tian_i4_start,
        .image_len = 16464U,
        .voice = s_tian_ima_start,
        .voice_len = 10616U
    },
    {
        .id = "yue",
        .hanzi = "月",
        .phrase = "月亮",
        .image = s_yue_i4_start,
        .image_len = 16464U,
        .voice = s_yue_ima_start,
        .voice_len = 8465U
    },
    {
        .id = "yu_rain",
        .hanzi = "雨",
        .phrase = "下雨",
        .image = s_yu_rain_i4_start,
        .image_len = 16464U,
        .voice = s_yu_rain_ima_start,
        .voice_len = 8745U
    },
    {
        .id = "hua",
        .hanzi = "花",
        .phrase = "小花",
        .image = s_hua_i4_start,
        .image_len = 16464U,
        .voice = s_hua_ima_start,
        .voice_len = 10468U
    },
    {
        .id = "shu_tree",
        .hanzi = "树",
        .phrase = "大树",
        .image = s_shu_tree_i4_start,
        .image_len = 16464U,
        .voice = s_shu_tree_ima_start,
        .voice_len = 8655U
    },
    {
        .id = "hong",
        .hanzi = "红",
        .phrase = "红苹果",
        .image = s_hong_i4_start,
        .image_len = 16464U,
        .voice = s_hong_ima_start,
        .voice_len = 11853U
    },
    {
        .id = "huang",
        .hanzi = "黄",
        .phrase = "黄香蕉",
        .image = s_huang_i4_start,
        .image_len = 16464U,
        .voice = s_huang_ima_start,
        .voice_len = 12883U
    },
    {
        .id = "lan",
        .hanzi = "蓝",
        .phrase = "蓝天空",
        .image = s_lan_i4_start,
        .image_len = 16464U,
        .voice = s_lan_ima_start,
        .voice_len = 12364U
    },
    {
        .id = "da",
        .hanzi = "大",
        .phrase = "大球",
        .image = s_da_i4_start,
        .image_len = 16464U,
        .voice = s_da_ima_start,
        .voice_len = 9128U
    },
    {
        .id = "xiao",
        .hanzi = "小",
        .phrase = "小球",
        .image = s_xiao_i4_start,
        .image_len = 16464U,
        .voice = s_xiao_ima_start,
        .voice_len = 11440U
    },
    {
        .id = "shang",
        .hanzi = "上",
        .phrase = "上楼",
        .image = s_shang_i4_start,
        .image_len = 16464U,
        .voice = s_shang_ima_start,
        .voice_len = 10234U
    },
    {
        .id = "xia",
        .hanzi = "下",
        .phrase = "下楼",
        .image = s_xia_i4_start,
        .image_len = 16464U,
        .voice = s_xia_ima_start,
        .voice_len = 9954U
    },
    {
        .id = "kai",
        .hanzi = "开",
        .phrase = "开门",
        .image = s_kai_i4_start,
        .image_len = 16464U,
        .voice = s_kai_ima_start,
        .voice_len = 9875U
    },
    {
        .id = "guan",
        .hanzi = "关",
        .phrase = "关门",
        .image = s_guan_i4_start,
        .image_len = 16464U,
        .voice = s_guan_ima_start,
        .voice_len = 9551U
    },
    {
        .id = "chi",
        .hanzi = "吃",
        .phrase = "吃饭",
        .image = s_chi_i4_start,
        .image_len = 16464U,
        .voice = s_chi_ima_start,
        .voice_len = 9631U
    },
    {
        .id = "he",
        .hanzi = "喝",
        .phrase = "喝水",
        .image = s_he_i4_start,
        .image_len = 16464U,
        .voice = s_he_ima_start,
        .voice_len = 9955U
    },
    {
        .id = "pao",
        .hanzi = "跑",
        .phrase = "跑一跑",
        .image = s_pao_i4_start,
        .image_len = 16464U,
        .voice = s_pao_ima_start,
        .voice_len = 10698U
    },
    {
        .id = "tiao",
        .hanzi = "跳",
        .phrase = "跳一跳",
        .image = s_tiao_i4_start,
        .image_len = 16464U,
        .voice = s_tiao_ima_start,
        .voice_len = 11359U
    }
};

int cards_catalog_get(uint8_t index, const cards_entry_t **entry)
{
    if (!entry || index >= CARDS_COUNT) {
        return 0;
    }
    *entry = &cards_catalog[index];
    return 1;
}
