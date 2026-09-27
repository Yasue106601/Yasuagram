import org.telegram.ui.Components.YasuVoiceTextParser;

public class TestIraqiNumbers {

    static int total = 0;
    static int failed = 0;

    static void test(String input, String expected) {
        total++;

        String actual = YasuVoiceTextParser.parseNumbersOnly(input);

        if (!expected.equals(actual)) {
            failed++;
            System.out.println(
                "FAIL | " + input +
                " | expected=" + expected +
                " | actual=" + actual
            );
        }
    }

    static String fmt(int value) {
        return YasuVoiceTextParser.parseNumbersOnly(String.valueOf(value));
    }

    public static void main(String[] args) {

        /*
         * =========================
         * Exact Iraqi dictionary 1-19
         * =========================
         */
        String[] oneTo19 = {
            "واحد",
            "اثنين",
            "كلاثة",
            "اربعة",
            "خمسة",
            "ستة",
            "سبعة",
            "ثمانية",
            "تسعة",
            "عشرة",
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

        for (int i = 0; i < oneTo19.length; i++) {
            test(oneTo19[i], fmt(i + 1));
        }

        /*
         * =========================
         * Exact Iraqi tens 20-99
         * =========================
         */
        String[] ones = {
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

        for (int t = 2; t <= 9; t++) {
            int base = t * 10;

            test(tens[t], fmt(base));

            for (int u = 1; u <= 9; u++) {
                test(
                    ones[u] + " و" + tens[t],
                    fmt(base + u)
                );
            }
        }

        /*
         * =========================
         * Exact Iraqi hundreds
         * =========================
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

        for (int h = 1; h <= 9; h++) {
            test(hundreds[h], fmt(h * 100));
        }

        /*
         * =========================
         * Hundreds + 1..99
         * =========================
         */
        for (int h = 1; h <= 9; h++) {
            int base = h * 100;

            for (int n = 1; n <= 99; n++) {
                String remainder;

                if (n <= 19) {
                    remainder = oneTo19[n - 1];
                } else {
                    int t = n / 10;
                    int u = n % 10;

                    if (u == 0) {
                        remainder = tens[t];
                    } else {
                        remainder = ones[u] + " و" + tens[t];
                    }
                }

                test(
                    hundreds[h] + " و" + remainder,
                    fmt(base + n)
                );
            }
        }

        /*
         * =========================
         * 1000
         * =========================
         */
        test("الف", "1.000");
        test("ألف", "1.000");
        test("آلف", "1.000");

        /*
         * =========================
         * Higher ranks
         * =========================
         */
        test("خمسة ألف", "5.000");
        test("خمسة آلاف", "5.000");
        test("عشرة آلاف", "10.000");

        test("مليون", "1.000.000");
        test("مليون و واحد", "1.000.001");
        test("خمسة مليون", "5.000.000");
        test("خمسة ملايين", "5.000.000");

        test("مليار", "1.000.000.000");
        test("خمسة مليار", "5.000.000.000");

        test("تريليون", "1.000.000.000.000");
        test("خمسة تريليون", "5.000.000.000.000");

        /*
         * =========================
         * Mixed ranks
         * =========================
         */
        test(
            "خمسة مليار وكلاثمية وخمسة وعشرين مليون",
            "5.325.000.000"
        );

        test(
            "مليار ومية وخمسة وعشرين مليون",
            "1.125.000.000"
        );

        /*
         * =========================
         * Numeric digits
         * =========================
         */
        test("12135", "12.135");
        test("123125003101", "123.125.003.101");

        /*
         * =========================
         * Text around numbers
         * =========================
         */
        test("الكلام 12135", "12.135");
        test("12135 كلام عادي", "12.135");
        test("السلام عليكم خمسة مليون شخص", "5.000.000");

        /*
         * =========================
         * Final result
         * =========================
         */
        System.out.println();
        System.out.println("TOTAL TESTS : " + total);
        System.out.println("FAILED      : " + failed);
        System.out.println(
            "PASSED      : " + (total - failed)
        );

        if (failed == 0) {
            System.out.println("STATUS      : ALL TESTS PASSED");
        } else {
            System.out.println("STATUS      : NEEDS FIXES");
            System.exit(1);
        }
    }
}
