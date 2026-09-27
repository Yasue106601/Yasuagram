import org.telegram.ui.Components.YasuVoiceTextParser;

public class YasuVoiceIntegrationTest {

    private static int passed = 0;
    private static int failed = 0;

    private static void check(String name, String raw, boolean isFinal,
                              int mode, String expected) {
        String output;

        if (mode == 1) {
            output = YasuVoiceTextParser.parseNumbersOnly(raw);
        } else {
            output = YasuVoiceTextParser.cleanWords(raw);
        }

        if (output.equals(expected)) {
            passed++;
            System.out.println("PASS  " + name + " -> [" + output + "]");
        } else {
            failed++;
            System.out.println("FAIL  " + name
                    + " raw=[" + raw + "]"
                    + " expected=[" + expected + "]"
                    + " got=[" + output + "]");
        }
    }

    public static void main(String[] args) {

        System.out.println("=== YASU VOICE INTEGRATION TEST ===");

        // 1. Partial numeric result.
        check(
                "partial number",
                "خمسة",
                false,
                1,
                "5"
        );

        // 2. Partial replacement.
        check(
                "partial expanded number",
                "خمسة وعشرين",
                false,
                1,
                "25"
        );

        // 3. Final numeric result.
        check(
                "final number",
                "خمسة وعشرين",
                true,
                1,
                "25"
        );

        // 4. Raw ASR words must never survive number parsing.
        check(
                "number + ordinary word",
                "خمسة وعشرين كتاب",
                false,
                1,
                "25"
        );

        // 5. Ordinary words only -> NOTHING.
        check(
                "ordinary words only",
                "كتاب جميل",
                false,
                1,
                ""
        );

        // 6. Exact Iraqi numeric vocabulary.
        check(
                "Iraqi 103",
                "مية وكلاثة",
                true,
                1,
                "103"
        );

        // 7. Thousands formatting.
        check(
                "spoken thousand",
                "خمسة ألف",
                true,
                1,
                "5.000"
        );

        // 8. Large digit run formatting.
        check(
                "digit formatting",
                "12345",
                true,
                1,
                "12.345"
        );

        // 9. Very large digit run formatting.
        check(
                "large digit formatting",
                "123125003101",
                true,
                1,
                "123.125.003.101"
        );

        // 10. Final unknown result -> empty, therefore no field update.
        check(
                "final unknown",
                "سلام عليكم",
                true,
                1,
                ""
        );

        // 11. Words mode remains normal.
        check(
                "words mode",
                "مرحبا يا صديقي",
                true,
                0,
                "مرحبا يا صديقي"
        );

        System.out.println();
        System.out.println("TOTAL : " + (passed + failed));
        System.out.println("PASS  : " + passed);
        System.out.println("FAIL  : " + failed);

        if (failed == 0) {
            System.out.println("STATUS: PASS");
        } else {
            System.out.println("STATUS: FAIL");
            System.exit(1);
        }
    }
}
