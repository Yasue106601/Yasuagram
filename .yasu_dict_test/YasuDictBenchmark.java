import org.telegram.ui.Components.YasuVoiceNumberDictionaries;

public class YasuDictBenchmark {
    private static void test(String text, int expected) {
        Integer value = YasuVoiceNumberDictionaries.lookup(text);

        System.out.println(
            text + " -> " + value +
            (value != null && value == expected
                ? " [OK]"
                : " [FAIL expected=" + expected + "]")
        );
    }

    public static void main(String[] args) {
        System.out.println("===== YASU DICTIONARY TEST =====");

        test("واحد", 1);
        test("اثنين", 2);
        test("عشرة", 10);
        test("عشرين", 20);
        test("مية", 100);
        test("مية وخمسة وعشرين", 125);
        test("ثلاثمية وسبعة وعشرين", 327);
        test("خمسمية وستين", 560);
        test("سبعمية وثلاثة عشر", 713);
        test("سبعمية وثلاثه عشر", 713);
        test("سبعمية وثلاثطعش", 713);
        test("ثمنمية وتسعين", 890);
        test("ثمنميه وتسعين", 890);

        System.out.println("===== LOOKUP SPEED =====");

        String[] samples = {
            "مية وخمسة وعشرين",
            "ثلاثمية وسبعة وعشرين",
            "خمسمية وستين",
            "سبعمية وثلاثة عشر",
            "ثمنمية وتسعين"
        };

        int loops = 100000;

        long start = System.nanoTime();

        int found = 0;

        for (int i = 0; i < loops; i++) {
            Integer value =
                YasuVoiceNumberDictionaries.lookup(
                    samples[i % samples.length]
                );

            if (value != null) {
                found++;
            }
        }

        long elapsed = System.nanoTime() - start;

        System.out.println("Lookups : " + loops);
        System.out.println("Found   : " + found);
        System.out.println("Total ms: " + (elapsed / 1_000_000.0));
        System.out.println(
            "Avg us  : " +
            (elapsed / (double) loops / 1000.0)
        );

        System.out.println("===== DONE =====");
    }
}
