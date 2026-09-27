import org.telegram.ui.Components.YasuVoiceTextParser;

public class TestYasuParser {
    private static void t(String input) {
        System.out.println("IN : " + input);
        System.out.println("OUT: " + YasuVoiceTextParser.parseNumbersOnly(input));
        System.out.println();
    }

    public static void main(String[] args) {
        t("12135");
        t("123125003101");

        t("واحد");
        t("خمسة عشر");
        t("مائة وعشرون");
        t("ثلاثمائة وخمسة وعشرون");

        t("خمسة ألف");
        t("خمسة مليون");
        t("خمسة مليار");
        t("خمسة تريليون");

        t("خمسة ديشليار");
        t("خمسة ديشليون");
        t("خمسة تيفليار");
        t("خمسة تيفليون");
        t("خمسة ويتليار");
        t("خمسة ويتليون");
        t("خمسة سيتليار");
        t("خمسة سيتليون");
        t("خمسة سيزليار");
        t("خمسة سيزيلون");
        t("خمسة سكليار");
        t("خمسة سكليون");
        t("خمسة كرليار");
        t("خمسة كرليون");
        t("خمسة ترليار");
        t("خمسة تريليون");
        t("خمسة بليار");
        t("خمسة بليون");
        t("خمسة مليار");
        t("خمسة مليون");
        t("خمسة الف");

        t("السلام عليكم خمسة مليون شخص");
        t("الكلام العادي بدون أي رقم");
        t("123125003101 كلام عادي");
        t("كلام عادي 12135");
    }
}
