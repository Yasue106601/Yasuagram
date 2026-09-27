import org.telegram.ui.Components.YasuVoiceTextParser;

public class YasuVoiceWordsTest {
    private static int total = 0;
    private static int pass = 0;

    private static void test(String name, String input, String expected) {
        total++;
        String actual = YasuVoiceTextParser.cleanWords(input);

        if (expected.equals(actual)) {
            pass++;
            System.out.println("PASS  " + name + " -> [" + actual + "]");
        } else {
            System.out.println("FAIL  " + name
                    + " -> expected [" + expected
                    + "] actual [" + actual + "]");
        }
    }

    public static void main(String[] args) {
        System.out.println("=== YASU WORDS-ONLY STRICT TEST ===");

        test("normal words",
                "مرحبا يا صديقي",
                "مرحبا يا صديقي");

        test("European digits",
                "مرحبا 123 صديقي",
                "مرحبا صديقي");

        test("Arabic digits",
                "مرحبا ١٢٣ صديقي",
                "مرحبا صديقي");

        test("standalone number words",
                "مرحبا خمسة صديقي",
                "مرحبا صديقي");

        test("numeric rank",
                "مرحبا خمسة ألف صديقي",
                "مرحبا صديقي");

        test("attached numeric واو",
                "مرحبا وعشرين صديقي",
                "مرحبا صديقي");

        test("mixed sentence",
                "هذا 25 كتاب جميل",
                "هذا كتاب جميل");

        test("only numbers",
                "خمسة وعشرين",
                "");

        System.out.println();
        System.out.println("TOTAL : " + total);
        System.out.println("PASS  : " + pass);
        System.out.println("FAIL  : " + (total - pass));
        System.out.println("STATUS: " + (pass == total ? "PASS" : "FAIL"));
    }
}
