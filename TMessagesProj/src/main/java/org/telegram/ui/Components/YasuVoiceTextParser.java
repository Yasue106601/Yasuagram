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
            // ألف
            "الف", "ألف", "آلف", "الاف", "آلاف",
            "الفين", "ألفين",

            // مليون
            "مليون", "ملايين", "مليونه",
            "ميليون", "ميليونه", "مليونين",

            // مليار
            "مليار", "مليارات", "ملياره",
            "ميليار", "ميليارات", "ميلياره",

            // بليون / بليار
            "بليون", "بليونات", "بليونه",
            "بليونين",
            "بليار", "بليارات", "بلياره",
            "بليارين",

            // تريليون / ترليون
            "تريليون", "تريليونات", "تريليونه",
            "ترليون", "ترليونات", "ترليونه",

            // مراتب ASR الكبيرة التي طلبها المستخدم.
            "ديشليار", "ديشليون",
            "تيفليار", "تيفليون",
            "ويتليار", "ويتليون",
            "سيتليار", "سيتليون",
            "سيزليار", "سيزيلون", "سيزليون",
            "سكليار", "سكليون",
            "كرليار", "كرليون",
            "ترليار", "ترليون",

            // و = رابط عددي فقط.
            "و"
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

    private static void addIraqiNumber(int value, String phrase) {
        IRAQI_NUMBER_WORDS.put(
                compact(normalizeWord(phrase)),
                value
        );
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

        final int length = value.length();
        int groupDigits = 0;

        for (int i = 0; i < length; i++) {
            char c = value.charAt(i);

            if (c >= '0' && c <= '9') {
                groupDigits++;
                continue;
            }

            if (c != '.') {
                return false;
            }

            if (groupDigits == 0 || i + 3 >= length) {
                return false;
            }

            char c1 = value.charAt(i + 1);
            char c2 = value.charAt(i + 2);
            char c3 = value.charAt(i + 3);

            if (c1 < '0' || c1 > '9'
                    || c2 < '0' || c2 > '9'
                    || c3 < '0' || c3 > '9') {
                return false;
            }

            if (i + 4 < length && value.charAt(i + 4) != '.') {
                return false;
            }

            // The validated group contains exactly 3 digits.
            groupDigits = 3;
            i += 3;
        }

        return groupDigits > 0;
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

        /*
         * Spoken number parser.
         */
        String spoken = parseSpokenNumber(normalized);

        if (digitRun.isEmpty()) {
            return safeNumericOutput(spoken);
        }

        if (spoken.isEmpty()) {
            return safeNumericOutput(digitRun);
        }

        /*
         * Prefer the longer confident numeric representation.
         */
        if (digitRun.length() >= spoken.length()) {
            return safeNumericOutput(digitRun);
        }

        return safeNumericOutput(spoken);
    }

    public static String cleanWords(String input) {
        if (input == null || input.trim().isEmpty()) {
            return "";
        }

        String[] raw = input.split("\\s+");
        StringBuilder out = new StringBuilder(input.length());

        for (String item : raw) {
            String token = stripPunctuation(item);

            if (token.isEmpty()) {
                continue;
            }

            /*
             * WORDS-ONLY STRICT FILTER:
             * Never allow Arabic-Indic/European digits.
             */
            if (containsDigit(token)) {
                continue;
            }

            String normalized = normalizeWord(token);

            /*
             * Remove standalone numeric words/ranks.
             */
            if (isKnownNumericToken(normalized)
                    || isMiya(normalized)
                    || isNumericRankToken(normalized)) {
                continue;
            }

            /*
             * Handle attached Arabic و:
             * "وعشرين" -> numeric "و" + "عشرين"
             * "ومرحبا" remains a normal word.
             */
            if (normalized.length() > 1
                    && normalized.charAt(0) == 'و') {

                String rest = normalized.substring(1);

                if (isKnownNumericToken(rest)
                        || isMiya(rest)
                        || isNumericRankToken(rest)) {
                    continue;
                }
            }

            if (out.length() > 0) {
                out.append(' ');
            }

            out.append(token);
        }

        return out.toString().trim();
    }

    private static boolean containsDigit(String value) {
        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);

            if ((c >= '0' && c <= '9')
                    || (c >= '٠' && c <= '٩')
                    || (c >= '۰' && c <= '۹')) {
                return true;
            }
        }

        return false;
    }

    private static String parseSpokenNumber(String input) {
        String[] raw = input.split("\\s+");

        ArrayList<String> tokens = new ArrayList<>();

        for (String item : raw) {
            String token = normalizeWord(stripPunctuation(item));

            if (token.isEmpty()) {
                continue;
            }

            /*
             * Split an attached Arabic و only when the remaining
             * part is a known numeric token/rank.
             */
            if (token.length() > 1 && token.charAt(0) == 'و') {
                String rest = token.substring(1);

                if (isKnownNumericToken(rest)) {
                    tokens.add("و");
                    tokens.add(rest);
                    continue;
                }
            }

            tokens.add(token);
        }

        String best = "";
        int bestRecognized = 0;

        for (int start = 0; start < tokens.size(); start++) {
            ParseResult result = parseRegionDetailed(tokens, start);

            if (result.value.isEmpty()) {
                continue;
            }

            if (result.recognized > bestRecognized
                    || (result.recognized == bestRecognized
                    && result.value.length() > best.length())) {

                best = result.value;
                bestRecognized = result.recognized;
            }
        }

        return best;
    }

    private static boolean isKnownNumericToken(String token) {
        String n = compact(normalizeWord(token));

        return IRAQI_NUMBER_WORDS.containsKey(n)
                || SCALE.containsKey(n)
                || isNumericRankToken(n);
    }

    private static final class ParseResult {
        final String value;
        final int recognized;

        ParseResult(String value, int recognized) {
            this.value = value;
            this.recognized = recognized;
        }
    }

    private static ParseResult parseRegionDetailed(
            List<String> tokens,
            int start
    ) {
        BigInteger total = BigInteger.ZERO;
        BigInteger current = BigInteger.ZERO;

        boolean found = false;
        int recognized = 0;

        for (int i = start; i < tokens.size();) {
            String token = tokens.get(i);

            if (token.equals("و")) {
                if (found) {
                    i++;
                    continue;
                }
                break;
            }

            /*
             * Numeric rank has priority over the single-word
             * dictionary entry.
             *
             * Example:
             * "خمسة ألف" must be 5000, not 1005.
             * "ألف" is also present as 1000 in the 1..1000
             * dictionary, so ranks must be checked first.
             */
            String normalizedToken = compact(
                    normalizeWord(token)
            );

            BigInteger scale = SCALE.get(normalizedToken);

            if (scale == null) {
                scale = getNumericRankScale(normalizedToken);
            }

            if (scale != null) {
                if (current.signum() == 0) {
                    current = BigInteger.ONE;
                }

                total = total.add(
                        current.multiply(scale)
                );

                current = BigInteger.ZERO;
                found = true;
                recognized++;
                i++;
                continue;
            }

            /*
             * A complete Iraqi 1..1000 phrase has priority.
             */
            int bestLength = 0;
            int bestValue = 0;

            int maxLength = Math.min(5, tokens.size() - i);

            for (int length = maxLength; length >= 1; length--) {
                StringBuilder phrase = new StringBuilder();

                for (int j = 0; j < length; j++) {
                    if (j > 0) {
                        phrase.append(' ');
                    }
                    phrase.append(tokens.get(i + j));
                }

                String key = compact(
                        normalizeWord(phrase.toString())
                );

                Integer value = IRAQI_NUMBER_WORDS.get(key);

                if (value != null) {
                    bestLength = length;
                    bestValue = value;
                    break;
                }
            }

            if (bestLength > 0) {
                current = current.add(
                        BigInteger.valueOf(bestValue)
                );

                found = true;
                recognized++;
                i += bestLength;
                continue;
            }

            /*
             * Unknown large ranks remain structural only.
             */
            if (isForbiddenScale(normalizedToken)) {
                if (found) {
                    break;
                }

                i++;
                continue;
            }

            if (isNumericRankToken(normalizedToken)) {
                found = true;
                recognized++;
                i++;
                continue;
            }

            break;
        }

        if (!found || recognized == 0) {
            return new ParseResult("", 0);
        }

        BigInteger result = total.add(current);

        if (result.signum() < 0) {
            return new ParseResult("", 0);
        }

        return new ParseResult(
                result.toString(),
                recognized
        );
    }

    private static BigInteger getNumericRankScale(String n) {

        /*
         * Known ranks whose numeric magnitude is unambiguous
         * in the current parser.
         */
        if (matchesRank(n,
                "الف", "الاف", "الف", "الاف",
                "الفين")) {
            /*
             * ألفين is two thousand, not one thousand.
             */
            if (n.equals("الفين")) {
                return BigInteger.valueOf(2000L);
            }
            return THOUSAND;
        }

        if (matchesRank(n,
                "مليون", "ملايين", "مليونه",
                "ميليون", "ميليونه", "مليونين")) {
            /*
             * مليونين is two million.
             */
            if (n.equals("مليونين")) {
                return BigInteger.valueOf(2000000L);
            }
            return MILLION;
        }

        if (matchesRank(n,
                "مليار", "مليارات", "ملياره",
                "ميليار", "ميليارات", "ميلياره",
                "بليون", "بليونات", "بليونه",
                "بليونين")) {
            /*
             * Preserve the existing parser meaning of بليون
             * as the billion rank.
             */
            if (n.equals("بليونين")) {
                return BigInteger.valueOf(2000000000L);
            }
            return BILLION;
        }

        if (matchesRank(n,
                "تريليون", "تريليونات", "تريليونه",
                "ترليون", "ترليونات", "ترليونه")) {
            return TRILLION;
        }

        /*
         * Other YASU rank vocabulary is recognized as a rank
         * by isNumericRankToken(), but its magnitude is intentionally
         * not invented here.
         */
        return null;
    }

    private static boolean matchesRank(
            String token,
            String... variants
    ) {
        for (String variant : variants) {
            if (token.equals(compact(normalizeWord(variant)))) {
                return true;
            }
        }
        return false;
    }

    private static boolean isNumericRankToken(String token) {
        String n = compact(normalizeWord(token));

        if (n.equals("و")) {
            return true;
        }

        for (String candidate : YASU_NUMERIC_RANK_VOCABULARY_NORMALIZED) {
            if (n.equals(candidate)) {
                return true;
            }

            /*
             * Very conservative ASR tolerance:
             * allow one edit only for long rank words.
             * Never use substring matching because that could
             * turn ordinary Arabic words into numeric ranks.
             */
            if (n.length() >= 6
                    && Math.abs(n.length() - candidate.length()) <= 1
                    && levenshtein(n, candidate, 1) <= 1) {
                return true;
            }
        }

        return false;
    }

    private static String[] normalizeVocabulary(String[] vocabulary) {
        String[] normalized = new String[vocabulary.length];

        for (int i = 0; i < vocabulary.length; i++) {
            normalized[i] = compact(normalizeWord(vocabulary[i]));
        }

        return normalized;
    }

    private static boolean isMiya(String token) {
        return token.equals("مية")
                || token.equals("ميه")
                || token.equals("ميا")
                || token.equals("مئه")
                || token.equals("مائه")
                || token.equals("مئة")
                || token.equals("مائة");
    }

    private static boolean isForbiddenScale(String token) {
        String n = compact(token);

        for (String word : FORBIDDEN_SCALE_WORDS) {
            if (n.equals(compact(word))) {
                return true;
            }
        }

        /*
         * Deliberately conservative fuzzy matching.
         *
         * We only compare against known forbidden-scale artifacts,
         * never against ordinary words.
         */
        if (n.length() < 7) {
            return false;
        }

        for (String word : FORBIDDEN_SCALE_WORDS) {
            String candidate = compact(word);

            if (Math.abs(n.length() - candidate.length()) > 1) {
                continue;
            }

            if (levenshtein(n, candidate, 1) <= 1) {
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
}
