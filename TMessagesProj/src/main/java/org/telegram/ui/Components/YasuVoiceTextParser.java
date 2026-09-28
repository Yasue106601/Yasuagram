package org.telegram.ui.Components;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class YasuVoiceTextParser {

    private YasuVoiceTextParser() {
    }

    public static final int MODE_WORDS = 0;
    public static final int MODE_NUMBERS = 1;

    private static final BigInteger THOUSAND = BigInteger.valueOf(1000L);
    private static final BigInteger MILLION = BigInteger.valueOf(1000000L);
    private static final BigInteger BILLION = BigInteger.valueOf(1000000000L);
    private static final BigInteger TRILLION = BigInteger.valueOf(1000000000000L);

    /*
     * Iraqi Arabic numeric dictionary.
     *
     * This is the source of truth for spoken numbers 1..1000.
     * Each complete Iraqi number phrase is stored as one numeric
     * entry. Higher ranks remain separate structural tokens.
     */
    private static final Map<String, Integer> IRAQI_NUMBER_WORDS = new HashMap<>();
    private static final Map<String, Integer> FUSHA_NUMBER_WORDS = new HashMap<>();
    private static final Map<String, Integer> FUSHA_SINGLE_NUMBER_WORDS = new HashMap<>();

    private static final Map<String, BigInteger> SCALE = new HashMap<>();

    /*
     * Words which must NEVER be treated as numeric scales.
     * They are common ASR-like artifacts around very large
     * number words and must never leak into numeric output.
     */

    /*
     * YASU_NUMERIC_RANK_VOCABULARY
     *
     * These are numeric rank words only.
     * They are structural markers and must never be emitted as text.
     *
     * Do NOT add ordinary Arabic words here.
     */
    private static final String[] YASU_NUMERIC_RANK_VOCABULARY = {
            // ديشليار
            "ديشليار", "ديشليارات", "ديشلياره", "ديشليارين",
            "ديشليون", "ديشليونات", "ديشليونه", "ديشليونين",

            // تيفليار
            "تيفليار", "تيفليارات", "تيفلياره", "تيفليارين",
            "تيفليون", "تيفليونات", "تيفليونه", "تيفليونين",

            // ويتليار
            "ويتليار", "ويتليارات", "ويتلياره", "ويتليارين",
            "ويتليون", "ويتليونات", "ويتليونه", "ويتليونين",

            // سيتليار
            "سيتليار", "سيتليارات", "سيتلياره", "سيتليارين",
            "سيتليون", "سيتليونات", "سيتليونه", "سيتليونين",

            // سيزليار
            "سيزليار", "سيزليارات", "سيزلياره", "سيزليارين",
            "سيزيلون", "سيزيلونات", "سيزيلونه", "سيزيلونين",
            "سيزليون", "سيزليونات", "سيزليونه", "سيزليونين",

            // سكليار
            "سكليار", "سكليارات", "سكلياره", "سكليارين",
            "سكليون", "سكليونات", "سكليونه", "سكليونين",

            // كرليار
            "كرليار", "كرليارات", "كرلياره", "كرليارين",
            "كرليون", "كرليونات", "كرليونه", "كرليونين",

            // ترليار / ترليون
            "ترليار", "ترليارات", "ترلياره", "ترليارين",
            "ترليون", "ترليونات", "ترليونه", "ترليونين",

            // تريليار / تريليون
            "تريليار", "تريليارات", "تريلياره", "تريليارين",
            "تريليون", "تريليونات", "تريليونه", "تريليونين",

            // بليار
            "بليار", "بليارات", "بلياره", "بليارين",
            "بليون", "بليونات", "بليونه", "بليونين",

            // مليار
            "مليار", "مليارات", "ملياره", "مليارين",
            "ميليار", "ميليارات", "ميلياره", "ميليارين",

            // مليون
            "مليون", "ملايين", "مليونه", "مليونا", "مليونين",
            "ميليون", "ميليونه", "ميليونات", "ميليونين",

            // ألف
            "ألف", "الف", "آلف", "الاف", "آلاف",
            "الفين", "ألفين",

            // الرابط العددي
    };

    private static final String[] YASU_NUMERIC_RANK_VOCABULARY_NORMALIZED =
            normalizeVocabulary(YASU_NUMERIC_RANK_VOCABULARY);

    private static final String[] FORBIDDEN_SCALE_WORDS = {
            "ديشليار",
            "ديشليون",
            "تيفليار",
            "تيفليون",
            "ويتليار",
            "ويتليون",
            "سيتليار",
            "سيتليون",
            "سيزليار",
            "سيزيلون",
            "سيزليون",
            "سكليار",
            "سكليون",
            "كرليار",
            "كرليون",
            "ترليار",
            "ترليون"
    };

    static {
        /*
         * Exact Iraqi 1..99 vocabulary supplied for Yasuagram.
         */
        String[] oneToNine = {
                "",
                "واحد",
                "اثنين",
                "كلاثة",
                "اربعة",
                "خمسة",
                "ستة",
                "سبعة",
                "ثمانية",
                "تسعة"
        };

        String[] tens = {
                "",
                "",
                "عشرين",
                "كلاثين",
                "اربعين",
                "خمسين",
                "ستين",
                "سبعين",
                "ثمانين",
                "تسعين"
        };

        String[] teens = {
                "",
                "اهدعش",
                "اثنعش",
                "كلطعش",
                "اربعطعش",
                "خمسطعش",
                "صطعش",
                "سبعطعش",
                "ثمنطعش",
                "تسعطعش"
        };

        addIraqiNumber(1, "واحد");
        addIraqiNumber(2, "اثنين");
        addIraqiNumber(3, "كلاثة");
        addIraqiNumber(4, "اربعة");
        addIraqiNumber(5, "خمسة");
        addIraqiNumber(6, "ستة");
        addIraqiNumber(7, "سبعة");
        addIraqiNumber(8, "ثمانية");
        addIraqiNumber(9, "تسعة");
        addIraqiNumber(10, "عشرة");

        for (int n = 11; n <= 19; n++) {
            addIraqiNumber(n, teens[n - 10]);
        }

        for (int n = 20; n <= 90; n += 10) {
            addIraqiNumber(n, tens[n / 10]);
        }

        for (int n = 21; n <= 99; n++) {
            if (n % 10 != 0) {
                addIraqiNumber(
                        n,
                        oneToNine[n % 10] + " و" + tens[n / 10]
                );
            }
        }

        /*
         * 100..999.
         *
         * Exact Iraqi hundreds supplied by the user:
         * 100 مية
         * 200 ميتين
         * 300 كلاثمية
         * 400 اربعمية
         * 500 خمسمية
         * 600 ستمية
         * 700 سبعمية
         * 800 ثمنمية
         * 900 تسعمية
         */
        String[] hundreds = {
                "",
                "مية",
                "ميتين",
                "كلاثمية",
                "اربعمية",
                "خمسمية",
                "ستمية",
                "سبعمية",
                "ثمنمية",
                "تسعمية"
        };

        addIraqiNumber(100, "مية");
        addIraqiNumber(200, "ميتين");
        addIraqiNumber(300, "كلاثمية");
        addIraqiNumber(400, "اربعمية");
        addIraqiNumber(500, "خمسمية");
        addIraqiNumber(600, "ستمية");
        addIraqiNumber(700, "سبعمية");
        addIraqiNumber(800, "ثمنمية");
        addIraqiNumber(900, "تسعمية");

        for (int h = 1; h <= 9; h++) {
            int base = h * 100;

            for (int n = 1; n <= 99; n++) {
                addIraqiNumber(
                        base + n,
                        hundreds[h] + " و" + getIraqiBaseNumber(n, oneToNine, teens, tens)
                );
            }
        }

        /*
         * 1000.
         */
        addIraqiNumber(1000, "الف");
        buildFushaNumberDictionary();

        /*
         * Real numeric ranks.
         * Rank words are structural and never emitted.
         */
        addScale(THOUSAND,
                "ألف",
                "الف",
                "آلف",
                "الاف",
                "آلاف"
        );

        addScale(MILLION,
                "مليون",
                "ملايين",
                "مليونه",
                "مليونا",
                "ميليون",
                "ميليونه"
        );

        addScale(BILLION,
                "مليار",
                "مليارات",
                "ملياره",
                "ميليار",
                "بليون",
                "بليونات",
                "بليونه",
                "بليار",
                "بليارات",
                "بلياره"
        );

        addScale(TRILLION,
                "تريليون",
                "تريليونات",
                "تريليونه",
                "ترليون",
                "ترليونات",
                "ترليونه"
        );
    }

    /*
     * Standard Arabic / Fusha number vocabulary.
     *
     * This mirrors the complete 1..1000 numeric vocabulary supplied
     * for NUMBER mode. It is kept separate from the Iraqi dictionary
     * so both dialects can be recognized independently.
     */
    private static void buildFushaNumberDictionary() {
        String[] units = {
                "",
                "واحد",
                "اثنان",
                "ثلاثة",
                "أربعة",
                "خمسة",
                "ستة",
                "سبعة",
                "ثمانية",
                "تسعة"
        };

        String[] teens = {
                "",
                "أحد عشر",
                "اثنا عشر",
                "ثلاثة عشر",
                "أربعة عشر",
                "خمسة عشر",
                "ستة عشر",
                "سبعة عشر",
                "ثمانية عشر",
                "تسعة عشر"
        };

        String[] tens = {
                "",
                "",
                "عشرون",
                "ثلاثون",
                "أربعون",
                "خمسون",
                "ستون",
                "سبعون",
                "ثمانون",
                "تسعون"
        };

        String[] hundreds = {
                "",
                "مائة",
                "مائتان",
                "ثلاثمائة",
                "أربعمائة",
                "خمسمائة",
                "ستمائة",
                "سبعمائة",
                "ثمانمائة",
                "تسعمائة"
        };

        /*
         * 1..10
         */
        for (int n = 1; n <= 10; n++) {
            addFushaNumber(n, n == 10 ? "عشرة" : units[n]);
        }

        /*
         * 11..19
         */
        for (int n = 11; n <= 19; n++) {
            addFushaNumber(n, teens[n - 10]);
        }

        /*
         * 20,30,...90
         */
        for (int n = 20; n <= 90; n += 10) {
            addFushaNumber(n, tens[n / 10]);
        }

        /*
         * 21..99
         *
         * Keep the supplied Fusha forms:
         * "اثنان وعشرون", "ثلاثة وثلاثون", etc.
         */
        for (int n = 21; n <= 99; n++) {
            if (n % 10 != 0) {
                int unit = n % 10;
                int ten = n / 10;

                addFushaNumber(
                        n,
                        units[unit] + " و" + tens[ten]
                );
            }
        }

        /*
         * Exact hundreds.
         */
        for (int h = 1; h <= 9; h++) {
            addFushaNumber(h * 100, hundreds[h]);
        }

        /*
         * 101..999
         */
        for (int n = 101; n <= 999; n++) {
            int h = n / 100;
            int rest = n % 100;

            if (rest == 0) {
                continue;
            }

            addFushaNumber(
                    n,
                    hundreds[h] + " و" + getFushaBaseNumber(rest, units, teens, tens)
            );
        }

        /*
         * 1000
         */
        addFushaNumber(1000, "ألف");
    }

    private static void addFushaNumber(int value, String phrase) {
        String normalized = compact(normalizeWord(phrase));

        FUSHA_NUMBER_WORDS.put(
                normalized,
                value
        );

        /*
         * Keep only genuine single-word Fusha forms for token-level
         * fuzzy matching. Composite phrases such as "واحد وعشرون"
         * are handled by the streaming parser component-by-component.
         */
        if (phrase.indexOf(' ') < 0) {
            FUSHA_SINGLE_NUMBER_WORDS.put(
                    normalized,
                    value
            );
        }
    }

    private static String getFushaBaseNumber(
            int n,
            String[] units,
            String[] teens,
            String[] tens
    ) {
        if (n >= 1 && n <= 9) {
            return units[n];
        }

        if (n == 10) {
            return "عشرة";
        }

        if (n >= 11 && n <= 19) {
            return teens[n - 10];
        }

        if (n % 10 == 0) {
            return tens[n / 10];
        }

        return units[n % 10] + " و" + tens[n / 10];
    }

    private static void addIraqiNumber(int value, String phrase) {
        IRAQI_NUMBER_WORDS.put(
                compact(normalizeWord(phrase)),
                value
        );
    }

    /*
     * Streaming number parser primitives.
     *
     * These are intentionally small building blocks, not a dictionary
     * containing every possible number.  Composite numbers are built
     * from these components at runtime.
     */
    private static final Map<String, Integer> YASU_STREAM_UNITS = new HashMap<>();
    private static final Map<String, Integer> YASU_STREAM_TENS = new HashMap<>();
    private static final Map<String, Integer> YASU_STREAM_HUNDREDS = new HashMap<>();

    static {
        addStreamValue(YASU_STREAM_UNITS, 1,
                "واحد", "وحده");
        addStreamValue(YASU_STREAM_UNITS, 2,
                "اثنين", "اثنين");
        addStreamValue(YASU_STREAM_UNITS, 3,
                "كلاثة", "ثلاثة", "ثلاثه");
        addStreamValue(YASU_STREAM_UNITS, 4,
                "اربعة", "أربعة", "اربعه", "أربعه");
        addStreamValue(YASU_STREAM_UNITS, 5,
                "خمسة", "خمسه");
        addStreamValue(YASU_STREAM_UNITS, 6,
                "ستة", "سته");
        addStreamValue(YASU_STREAM_UNITS, 7,
                "سبعة", "سبعه");
        addStreamValue(YASU_STREAM_UNITS, 8,
                "ثمانية", "ثمانيه");
        addStreamValue(YASU_STREAM_UNITS, 9,
                "تسعة", "تسعه");

        addStreamValue(YASU_STREAM_TENS, 20,
                "عشرين");
        addStreamValue(YASU_STREAM_TENS, 30,
                "كلاثين", "ثلاثين");
        addStreamValue(YASU_STREAM_TENS, 40,
                "اربعين", "أربعين");
        addStreamValue(YASU_STREAM_TENS, 50,
                "خمسين");
        addStreamValue(YASU_STREAM_TENS, 60,
                "ستين");
        addStreamValue(YASU_STREAM_TENS, 70,
                "سبعين");
        addStreamValue(YASU_STREAM_TENS, 80,
                "ثمانين");
        addStreamValue(YASU_STREAM_TENS, 90,
                "تسعين");

        addStreamValue(YASU_STREAM_HUNDREDS, 100,
                "مية", "ميه", "ميا", "مئه", "مائه", "مئة", "مائة");
        addStreamValue(YASU_STREAM_HUNDREDS, 200,
                "ميتين", "مئتين", "مائتين");
        addStreamValue(YASU_STREAM_HUNDREDS, 300,
                "كلاثمية", "ثلاثمية", "ثلاثمئة", "ثلاثمائة");
        addStreamValue(YASU_STREAM_HUNDREDS, 400,
                "اربعمية", "أربعمية", "أربعمئة", "أربعمائة");
        addStreamValue(YASU_STREAM_HUNDREDS, 500,
                "خمسمية", "خمسمئة", "خمسمائة");
        addStreamValue(YASU_STREAM_HUNDREDS, 600,
                "ستمية", "ستمئة", "ستمائة");
        addStreamValue(YASU_STREAM_HUNDREDS, 700,
                "سبعمية", "سبعمئة", "سبعمائة");
        addStreamValue(YASU_STREAM_HUNDREDS, 800,
                "ثمنمية", "ثمانمية", "ثمانمئة", "ثمانمائة");
        addStreamValue(YASU_STREAM_HUNDREDS, 900,
                "تسعمية", "تسعمئة", "تسعمائة");
    }

    /*
     * Parse one spoken numeric chunk in the range 0..999.
     *
     * The parser is compositional:
     *   hundreds + tens + units
     *
     * The word "و" is optional because ASR/speech may omit it.
     */
    private static int parseStreamNumberChunk(List<String> tokens) {
        if (tokens == null || tokens.isEmpty()) {
            return -1;
        }

        int value = 0;
        boolean found = false;

        for (String raw : tokens) {
            String token = compact(normalizeWord(raw));

            if (token.isEmpty() || token.equals("و")) {
                continue;
            }

            Integer hundred = YASU_STREAM_HUNDREDS.get(token);
            if (hundred != null) {
                value += hundred;
                found = true;
                continue;
            }

            Integer ten = YASU_STREAM_TENS.get(token);
            if (ten != null) {
                value += ten;
                found = true;
                continue;
            }

            Integer unit = YASU_STREAM_UNITS.get(token);
            if (unit != null) {
                value += unit;
                found = true;
                continue;
            }

            /*
             * Exact dictionary confirmation.
             *
             * The old dictionary is not used to generate the parser's
             * vocabulary. It only confirms an already numeric-looking
             * spoken token.
             */
            Integer confirmed = IRAQI_NUMBER_WORDS.get(token);
            if (confirmed != null && confirmed >= 1 && confirmed <= 999) {
                value += confirmed;
                found = true;
                continue;
            }

            /*
             * "عشرة" is kept separate because it is the base of the
             * 11..19 family and is useful as a standalone component.
             */
            if (token.equals("عشرة")) {
                value += 10;
                found = true;
                continue;
            }

            /*
             * Common Iraqi spoken teen forms.
             * These remain a small component vocabulary, not a
             * dictionary of every composite number.
             */
            Integer teen = parseStreamTeen(token);
            if (teen != null) {
                value += teen;
                found = true;
                continue;
            }

            return -1;
        }

        if (!found || value < 0 || value > 999) {
            return -1;
        }

        return value;
    }

    private static Integer parseStreamTeen(String token) {
        if (token == null) {
            return null;
        }

        String n = compact(normalizeWord(token));

        switch (n) {
            case "اهدعش":
            case "احدعش":
            case "احدعشر":
                return 11;

            case "اثنعش":
            case "اثنعشر":
                return 12;

            case "كلطعش":
            case "ثلطعش":
            case "ثلاثطعش":
                return 13;

            case "اربعطعش":
            case "اربعتعش":
                return 14;

            case "خمسطعش":
            case "خمستعش":
                return 15;

            case "صطعش":
            case "ستطعش":
            case "ستتعش":
                return 16;

            case "سبعطعش":
            case "سبعتعش":
                return 17;

            case "ثمنطعش":
            case "ثمنتعش":
            case "ثمانطعش":
                return 18;

            case "تسعطعش":
            case "تسعتعش":
                return 19;

            default:
                return null;
        }
    }

    private static void addStreamValue(
            Map<String, Integer> map,
            int value,
            String... words
    ) {
        for (String word : words) {
            map.put(compact(normalizeWord(word)), value);
        }
    }

    private static String getIraqiBaseNumber(
            int n,
            String[] oneToNine,
            String[] teens,
            String[] tens
    ) {
        if (n >= 1 && n <= 9) {
            return oneToNine[n];
        }

        if (n == 10) {
            return "عشرة";
        }

        if (n >= 11 && n <= 19) {
            return teens[n - 10];
        }

        if (n % 10 == 0) {
            return tens[n / 10];
        }

        return oneToNine[n % 10] + " و" + tens[n / 10];
    }

    private static void addScale(BigInteger value, String... words) {
        for (String word : words) {
            SCALE.put(
                    compact(normalizeWord(word)),
                    value
            );
        }
    }

    /**
     * Final fail-closed gate for numeric-only mode.
     *
     * Accepted:
     *   5
     *   25
     *   5.000
     *   12.345
     *   123.125.003.101
     *
     * Rejected:
     *   any Arabic/Latin letters
     *   spaces
     *   punctuation other than grouping dots
     *   malformed grouping
     */
    public static boolean isStrictNumericOutput(String value) {
        if (value == null || value.isEmpty()) {
            return false;
        }

        /*
         * Streaming numeric output may temporarily end with '.':
         *
         *   120.
         *   120.150.
         *
         * The trailing dot means that the previous 3-digit group
         * has been committed and the next rank/group is still being
         * spoken.
         */
        boolean trailingDot = value.charAt(value.length() - 1) == '.';

        String checked = trailingDot
                ? value.substring(0, value.length() - 1)
                : value;

        if (checked.isEmpty()) {
            return false;
        }

        String[] groups = checked.split("\\.", -1);

        if (groups.length == 0) {
            return false;
        }

        /*
         * First group: 1..3 digits.
         * Following groups: exactly 3 digits.
         */
        for (int i = 0; i < groups.length; i++) {
            String group = groups[i];

            if (group.isEmpty()) {
                return false;
            }

            int expected = i == 0 ? 3 : 3;

            if (group.length() > expected) {
                return false;
            }

            if (i > 0 && group.length() != 3) {
                return false;
            }

            for (int j = 0; j < group.length(); j++) {
                char c = group.charAt(j);

                if (c < '0' || c > '9') {
                    return false;
                }
            }
        }

        return true;
    }

    public static String parseNumbersOnly(String input) {
        if (input == null || input.trim().isEmpty()) {
            return "";
        }

        String normalized = normalize(input);

        /*
         * Explicit digits have the highest confidence.
         */
        String digitRun = extractBestDigitRun(normalized);
        if (!digitRun.isEmpty()) {
            return safeNumericOutput(digitRun);
        }

        /*
         * Streaming spoken-number parser.
         *
         * The important rule here is that ranks are STRUCTURAL
         * boundaries only. They do not have invented magnitudes.
         *
         * Example:
         *
         *   مية وعشرين بليار
         *   مية وخمسين بليون
         *   ميتين مليار
         *
         * becomes:
         *
         *   120.
         *   120.150
         *   120.150.200
         *
         * The large IRAQI_NUMBER_WORDS dictionary is deliberately
         * NOT used to construct composite numbers here.
         * It remains available as a recognition/confirmation aid.
         */
        return parseStreamingSpokenNumber(normalized);
    }

    private static String parseStreamingSpokenNumber(String input) {
        String[] raw = input.split("\\s+");

        ArrayList<String> tokens = new ArrayList<>();

        for (String item : raw) {
            String token = compact(normalizeWord(stripPunctuation(item)));

            if (token.isEmpty()) {
                continue;
            }

            /*
             * Split attached Arabic و only when the remainder is
             * a numeric component/rank.
             */
            if (token.length() > 1 && token.charAt(0) == 'و') {
                String rest = token.substring(1);

                if (isStreamingNumericComponent(rest)
                        || isMiya(rest)
                        || isNumericRankToken(rest)) {
                    tokens.add("و");
                    tokens.add(rest);
                    continue;
                }
            }

            tokens.add(token);
        }

        if (tokens.isEmpty()) {
            return "";
        }

        StringBuilder result = new StringBuilder();
        ArrayList<String> current = new ArrayList<>();

        boolean hasCommittedRank = false;
        boolean hasAnyGroup = false;

        for (String token : tokens) {
            if (token.equals("و")) {
                /*
                 * "و" is only a connector.
                 */
                continue;
            }

            if (isNumericRankToken(token)) {
                /*
                 * Rank = hard 3-digit group boundary.
                 */
                int value = parseStreamingChunk(current);

                if (value < 0) {
                    return "";
                }

                if (hasAnyGroup) {
                    /*
                     * Every group after the first is exactly 3 digits.
                     */
                    if (result.length() == 0
                            || result.charAt(result.length() - 1) != '.') {
                        result.append('.');
                    }
                }

                result.append(formatStreamingGroup(value, hasAnyGroup));

                /*
                 * Rank itself is already spoken.
                 *
                 * The separator before the next group is added
                 * when that next group is committed. Do not add
                 * another separator here, otherwise:
                 *
                 *   120..150
                 *
                 * would be produced.
                 */
                result.append('.');

                hasCommittedRank = true;
                hasAnyGroup = true;
                current.clear();
                continue;
            }

            if (isStreamingNumericComponent(token)) {
                String corrected =
                        correctStreamingNumericToken(token);

                current.add(
                        corrected != null
                                ? corrected
                                : token
                );
                continue;
            }

            /*
             * Unknown speech ends the numeric candidate.
             * Never extract a smaller valid substring.
             */
            break;
        }

        /*
         * Final group does not require a rank.
         *
         * If a rank was already committed, this becomes:
         *
         *   120.150
         *
         * instead of:
         *
         *   120.150.
         */
        if (!current.isEmpty()) {
            int value = parseStreamingChunk(current);

            if (value >= 0) {
                if (hasAnyGroup) {
                    /*
                     * The previous rank already left a trailing dot.
                     * Do not add another dot.
                     */
                    if (result.length() == 0
                            || result.charAt(result.length() - 1) != '.') {
                        result.append('.');
                    }
                }

                result.append(formatStreamingGroup(value, hasAnyGroup));

                /*
                 * A final unranked group is complete and therefore
                 * has no trailing dot.
                 */
                hasCommittedRank = false;
                hasAnyGroup = true;
            }
        }

        if (result.length() == 0) {
            return "";
        }

        return result.toString();
    }

    private static boolean isStreamingNumericComponent(String token) {
        if (token == null || token.isEmpty()) {
            return false;
        }

        String n = compact(normalizeWord(token));

        if (n.equals("و")) {
            return true;
        }

        if (YASU_STREAM_UNITS.containsKey(n)
                || YASU_STREAM_TENS.containsKey(n)
                || YASU_STREAM_HUNDREDS.containsKey(n)) {
            return true;
        }

        if (n.equals("عشرة")) {
            return true;
        }

        if (parseStreamTeen(n) != null) {
            return true;
        }

        /*
         * Moonshine can produce small spelling/phonetic errors,
         * especially with fast, clipped, or noisy speech.
         *
         * In NUMBER mode only, allow one conservative edit against
         * the existing numeric vocabulary. We never compare against
         * ordinary Arabic words.
         */
        return correctStreamingNumericToken(n) != null;
    }

    /*
     * Return the canonical numeric token for a conservative fuzzy
     * match, or null when there is no sufficiently safe match.
     *
     * Minimum length of 4 prevents very short Arabic words from being
     * accidentally interpreted as numbers.
     */
    private static String correctStreamingNumericToken(String token) {
        if (token == null || token.isEmpty()) {
            return null;
        }

        String n = compact(normalizeWord(token));

        if (n.isEmpty() || n.equals("و")) {
            return null;
        }

        /*
         * NUMBER mode is numeric-only.
         *
         * Rank words (ألف، مليون، مليار, etc.) are structural
         * and must NEVER become numeric components.
         *
         * Check them before fuzzy numeric matching so a misspelled
         * rank cannot accidentally match an ordinary number word.
         */
        if (isNumericRankToken(n) || isForbiddenScale(n)) {
            return null;
        }

        /*
         * Exact numeric vocabulary always wins.
         */
        if (YASU_STREAM_UNITS.containsKey(n)
                || YASU_STREAM_TENS.containsKey(n)
                || YASU_STREAM_HUNDREDS.containsKey(n)
                || n.equals("عشرة")
                || parseStreamTeen(n) != null) {
            return n;
        }

        /*
         * Strong fuzzy matching.
         *
         * Short words stay conservative.
         * Longer number words can tolerate up to two edits:
         * missing character, extra character, substitution, etc.
         *
         * We compare ONLY against existing numeric dictionaries.
         * No ordinary Arabic vocabulary is ever used.
         */
        int maxDistance;

        if (n.length() <= 5) {
            maxDistance = 1;
        } else {
            maxDistance = 2;
        }

        String best = null;
        int bestDistance = maxDistance + 1;
        boolean ambiguous = false;

        /*
         * Numeric component dictionaries.
         */
        String[][] dictionaries = {
                YASU_STREAM_UNITS.keySet().toArray(new String[0]),
                YASU_STREAM_TENS.keySet().toArray(new String[0]),
                YASU_STREAM_HUNDREDS.keySet().toArray(new String[0])
        };

        for (String[] dictionary : dictionaries) {
            for (String candidate : dictionary) {
                if (candidate == null || candidate.isEmpty()) {
                    continue;
                }

                /*
                 * Never allow a fuzzy match to a rank.
                 */
                if (isNumericRankToken(candidate)
                        || isForbiddenScale(candidate)) {
                    continue;
                }

                int allowed = Math.min(
                        maxDistance,
                        Math.max(1, candidate.length() / 4)
                );

                if (Math.abs(n.length() - candidate.length()) > allowed) {
                    continue;
                }

                int distance = levenshtein(n, candidate, allowed);

                if (distance > allowed) {
                    continue;
                }

                if (distance < bestDistance) {
                    best = candidate;
                    bestDistance = distance;
                    ambiguous = false;
                } else if (distance == bestDistance
                        && best != null
                        && !best.equals(candidate)) {
                    /*
                     * Two different numeric words are equally close.
                     * Do not guess in an ambiguous case.
                     */
                    ambiguous = true;
                }
            }
        }

        /*
         * Fusha + Iraqi generated number dictionaries.
         *
         * Both dictionaries participate in the same fuzzy search.
         * The parser works token-by-token, so composite Fusha phrases
         * containing the Arabic connector "و" are not used here.
         */
        Map<String, Integer>[] numberDictionaries = new Map[] {
                FUSHA_SINGLE_NUMBER_WORDS,
                YASU_STREAM_UNITS,
                YASU_STREAM_TENS,
                YASU_STREAM_HUNDREDS
        };

        for (Map<String, Integer> dictionary : numberDictionaries) {
            for (String candidate : dictionary.keySet()) {
                Integer value = dictionary.get(candidate);

                if (value == null || value < 1 || value > 999) {
                    continue;
                }

                if (candidate == null || candidate.isEmpty()) {
                    continue;
                }

                if (isNumericRankToken(candidate)
                        || isForbiddenScale(candidate)) {
                    continue;
                }

                int allowed = Math.min(
                        maxDistance,
                        Math.max(1, candidate.length() / 4)
                );

                if (Math.abs(n.length() - candidate.length()) > allowed) {
                    continue;
                }

                int distance = levenshtein(n, candidate, allowed);

                if (distance > allowed) {
                    continue;
                }

                if (distance < bestDistance) {
                    best = candidate;
                    bestDistance = distance;
                    ambiguous = false;
                } else if (distance == bestDistance
                        && best != null
                        && !best.equals(candidate)) {
                    /*
                     * Equal-distance different numeric words are unsafe.
                     */
                    ambiguous = true;
                }
            }
        }

        /*
         * No numeric candidate = emit nothing.
         *
         * Ambiguous candidate = emit nothing.
         *
         * NUMBER mode must prefer an empty result over inventing
         * a number from ordinary or uncertain speech.
         */
        if (best == null || ambiguous) {
            return null;
        }

        return best;
    }

    /*
     * Numeric rank detection.
     *
     * Rank words are structural markers:
     * ألف / مليون / مليار / ... .
     *
     * They must NEVER enter the numeric-component fuzzy matcher.
     * We therefore check forbidden scale artifacts first, then
     * the complete existing rank vocabulary.
     */
    /*
     * Iraqi spoken "hundred" component.
     *
     * These forms are numeric components and are handled separately
     * from the normal unit/ten/hundred vocabulary because "مية"
     * can be followed by another numeric component.
     */
    private static boolean isMiya(String token) {
        if (token == null || token.isEmpty()) {
            return false;
        }

        String n = compact(normalizeWord(token));

        return n.equals("مية")
                || n.equals("ميه")
                || n.equals("مئة")
                || n.equals("مائة")
                || n.equals("مائه");
    }

    private static boolean isNumericRankToken(String token) {
        if (token == null || token.isEmpty()) {
            return false;
        }

        String n = compact(normalizeWord(token));

        if (n.isEmpty() || n.equals("و")) {
            return false;
        }

        /*
         * Forbidden scale artifacts always win the exclusion.
         */
        if (isForbiddenScale(n)) {
            return false;
        }

        /*
         * Exact rank match.
         */
        for (String candidate :
                YASU_NUMERIC_RANK_VOCABULARY_NORMALIZED) {

            if (n.equals(candidate)) {
                return true;
            }
        }

        /*
         * Strong-but-controlled ASR tolerance for ranks.
         *
         * Short rank words: 1 edit.
         * Longer rank words: up to 2 edits.
         *
         * This happens BEFORE numeric fuzzy matching, so a word
         * resembling "مليون" or "مليار" is treated as a rank,
         * not as an ordinary number.
         */
        int maxDistance = n.length() <= 5 ? 1 : 2;

        for (String candidate :
                YASU_NUMERIC_RANK_VOCABULARY_NORMALIZED) {

            if (candidate == null || candidate.isEmpty()) {
                continue;
            }

            if (isForbiddenScale(candidate)) {
                continue;
            }

            int allowed = Math.min(
                    maxDistance,
                    Math.max(1, candidate.length() / 4)
            );

            if (Math.abs(n.length() - candidate.length()) > allowed) {
                continue;
            }

            if (levenshtein(n, candidate, allowed) <= allowed) {
                return true;
            }
        }

        return false;
    }

    /*
     * Words which must never become numeric ranks.
     *
     * Exact and close ASR variants are both blocked.
     */
    private static boolean isForbiddenScale(String token) {
        if (token == null || token.isEmpty()) {
            return false;
        }

        String n = compact(normalizeWord(token));

        if (n.isEmpty()) {
            return false;
        }

        for (String word : FORBIDDEN_SCALE_WORDS) {
            String candidate = compact(normalizeWord(word));

            if (n.equals(candidate)) {
                return true;
            }
        }

        /*
         * Strong fuzzy exclusion for the known forbidden scale
         * vocabulary. Never use ordinary Arabic words here.
         */
        int maxDistance = n.length() <= 6 ? 1 : 2;

        for (String word : FORBIDDEN_SCALE_WORDS) {
            String candidate = compact(normalizeWord(word));

            if (candidate.isEmpty()) {
                continue;
            }

            int allowed = Math.min(
                    maxDistance,
                    Math.max(1, candidate.length() / 4)
            );

            if (Math.abs(n.length() - candidate.length()) > allowed) {
                continue;
            }

            if (levenshtein(n, candidate, allowed) <= allowed) {
                return true;
            }
        }

        return false;
    }

    private static int levenshtein(
            String a,
            String b,
            int limit
    ) {
        if (Math.abs(a.length() - b.length()) > limit) {
            return limit + 1;
        }

        int[] prev = new int[b.length() + 1];
        int[] curr = new int[b.length() + 1];

        for (int j = 0; j <= b.length(); j++) {
            prev[j] = j;
        }

        for (int i = 1; i <= a.length(); i++) {
            curr[0] = i;

            int min = curr[0];

            for (int j = 1; j <= b.length(); j++) {
                int cost =
                        a.charAt(i - 1) == b.charAt(j - 1)
                                ? 0
                                : 1;

                curr[j] = Math.min(
                        Math.min(
                                curr[j - 1] + 1,
                                prev[j] + 1
                        ),
                        prev[j - 1] + cost
                );

                min = Math.min(min, curr[j]);
            }

            if (min > limit) {
                return limit + 1;
            }

            int[] tmp = prev;
            prev = curr;
            curr = tmp;
        }

        return prev[b.length()];
    }

    private static String extractBestDigitRun(String text) {
        String best = "";
        StringBuilder current = new StringBuilder();

        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);

            if (c >= '0' && c <= '9') {
                current.append(c);
            } else {
                if (current.length() > best.length()) {
                    best = current.toString();
                }

                current.setLength(0);
            }
        }

        if (current.length() > best.length()) {
            best = current.toString();
        }

        return best;
    }

    private static String safeNumericOutput(String digits) {
        if (digits == null || digits.isEmpty()) {
            return "";
        }

        StringBuilder onlyDigits = new StringBuilder();

        for (int i = 0; i < digits.length(); i++) {
            char c = digits.charAt(i);

            if (c >= '0' && c <= '9') {
                onlyDigits.append(c);
            }
        }

        if (onlyDigits.length() == 0) {
            return "";
        }

        return formatGrouped(onlyDigits.toString());
    }

    private static String formatGrouped(String digits) {
        if (digits == null || digits.isEmpty()) {
            return "";
        }

        if (digits.length() <= 3) {
            return digits;
        }

        int first = digits.length() % 3;

        if (first == 0) {
            first = 3;
        }

        StringBuilder result =
                new StringBuilder(
                        digits.length() + digits.length() / 3
                );

        result.append(
                digits,
                0,
                first
        );

        for (int i = first; i < digits.length(); i += 3) {
            result.append('.');

            result.append(
                    digits,
                    i,
                    Math.min(i + 3, digits.length())
            );
        }

        return result.toString();
    }

    private static String stripPunctuation(String value) {
        if (value == null || value.isEmpty()) {
            return "";
        }

        int start = 0;
        int end = value.length();

        while (start < end && isPunctuation(value.charAt(start))) {
            start++;
        }

        while (end > start && isPunctuation(value.charAt(end - 1))) {
            end--;
        }

        return value.substring(start, end);
    }

    private static boolean isPunctuation(char c) {
        switch (c) {
            case '،':
            case ',':
            case '.':
            case '!':
            case '?':
            case '؟':
            case ':':
            case ';':
            case '"':
            case '\'':
            case '(':
            case ')':
            case '[':
            case ']':
            case '{':
            case '}':
                return true;

            default:
                return false;
        }
    }

    private static String normalize(String value) {
        if (value == null) {
            return "";
        }

        return value
                .replace('٠', '0')
                .replace('١', '1')
                .replace('٢', '2')
                .replace('٣', '3')
                .replace('٤', '4')
                .replace('٥', '5')
                .replace('٦', '6')
                .replace('٧', '7')
                .replace('٨', '8')
                .replace('٩', '9')
                .replace('\u064B', ' ')
                .replace('\u064C', ' ')
                .replace('\u064D', ' ')
                .replace('\u064E', ' ')
                .replace('\u064F', ' ')
                .replace('\u0650', ' ')
                .replace('\u0651', ' ')
                .replace('\u0652', ' ')
                .replace('\u0670', ' ')
                .replace('\u0640', ' ')
                .trim();
    }

    private static String normalizeWord(String value) {
        if (value == null) {
            return "";
        }

        return normalize(value)
                .replace('أ', 'ا')
                .replace('إ', 'ا')
                .replace('آ', 'ا')
                .replace('ٱ', 'ا')
                .replace('ة', 'ه')
                .replace('ى', 'ي')
                .trim();
    }

    private static String compact(String value) {
        return normalizeWord(value)
                .replace(" ", "")
                .replace("-", "")
                .replace("_", "");
    }

    public static void yasuDebugNumberTests() {
        System.out.println("[YASU NUMBER SMART TEST] =====");

        String[] smartInputs = {
                "عندي ثلاثمية وسبعة وعشرين كتاب",
                "ثلاثمية وسبعة وعشرين كتاب",
                "اشتريت سبعمية وثلاثة عشر قطعة",
                "سبعمية وثلاثة عشر قطعة",
                "عندي ألف ومئتين وخمسة وأربعين",
                "ألف ومئتين وخمسة وأربعين",
                "أريد مية وخمسة وعشرين دولار",
                "مية وخمسة وعشرين دولار",
                "اليوم الجو حلو والشارع مزدحم",
                "هذا اختبار للكلمات العادية"
        };

        for (String input : smartInputs) {
            System.out.println(
                    "[YASU SMART] "
                            + input
                            + " -> "
                            + parseNumbersOnly(input)
            );
        }

        System.out.println("[YASU NUMBER SMART TEST] =====");

        String[] inputs = {
                "خمسمية وستين",
                "مية وعشرين بليار",
                "مية وعشرين بليار ومية وخمسين بليون",
                "مية وعشرين بليار ومية وخمسين بليون وميتين مليار",
                "مية وعشرين بليار ومية وخمسين بليون وميتين مليار وسبعين مليون",
                "مية وعشرين بليار ومية وخمسين بليون وميتين مليار وسبعين مليون ومية وستين الف ومية وسبعين",
                "خمسمية وستين مليار",
                "خمسمية وستين",
                "خمسمية وستين ملياار",
                "ميتين وثلاثين مليون",
                "مية وخمسين بليون",
                "مليار ألف مليون",
                "ثمنمية وتسعين",
                "ثمنميه وتسعين"
        };

        for (String input : inputs) {
            System.out.println(
                    "[YASU NUMBER TEST] "
                            + input
                            + " -> "
                            + parseNumbersOnly(input)
            );
        }
    }
}
