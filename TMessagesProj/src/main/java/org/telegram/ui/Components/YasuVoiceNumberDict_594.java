package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_594 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("ثمنيه", 8);
        IQ.put("ثمنيه عشر", 18);
        IQ.put("ثمنيه و عشرة", 18);
        IQ.put("ثمنيه و عشره", 18);
        IQ.put("ثمنيه وأربعين", 48);
        IQ.put("ثمنيه واربعين", 48);
        IQ.put("ثمنيه وتسعين", 98);
        IQ.put("ثمنيه وثلاثين", 38);
        IQ.put("ثمنيه وثمانين", 88);
        IQ.put("ثمنيه وثمنين", 88);
        IQ.put("ثمنيه وخمسين", 58);
        IQ.put("ثمنيه وسبعين", 78);
        IQ.put("ثمنيه وستين", 68);
        IQ.put("ثمنيه وعشرة", 18);
        IQ.put("ثمنيه وعشره", 18);
        IQ.put("ثمنيه وعشرين", 28);
        IQ.put("ثمنيه وكلاثين", 38);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}