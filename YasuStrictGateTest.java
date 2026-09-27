import org.telegram.ui.Components.YasuVoiceTextParser;

public class YasuStrictGateTest {
    static int pass = 0;
    static int fail = 0;

    static void test(String value, boolean expected) {
        boolean got = YasuVoiceTextParser.isStrictNumericOutput(value);

        if (got == expected) {
            pass++;
            System.out.println("PASS [" + value + "] -> " + got);
        } else {
            fail++;
            System.out.println("FAIL [" + value + "] expected="
                    + expected + " got=" + got);
        }
    }

    public static void main(String[] args) {

        System.out.println("=== STRICT NUMERIC GATE ===");

        test("5", true);
        test("25", true);
        test("103", true);
        test("5.000", true);
        test("12.345", true);
        test("123.125.003.101", true);
        test("000", true);

        test("", false);
        test("خمسة", false);
        test("خمسة وعشرين", false);
        test("25 كتاب", false);
        test("كتاب 25", false);
        test("hello", false);
        test("12.34", false);
        test("12.345.6", false);
        test("12.3456", false);
        test("12..345", false);
        test("12 345", false);
        test("١٢٣", false);
        test("25.", false);
        test(".25", false);

        System.out.println();
        System.out.println("TOTAL : " + (pass + fail));
        System.out.println("PASS  : " + pass);
        System.out.println("FAIL  : " + fail);

        if (fail == 0) {
            System.out.println("STATUS: PASS");
        } else {
            System.out.println("STATUS: FAIL");
            System.exit(1);
        }
    }
}
