package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_592 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("ثمنية", 8);
        IQ.put("ثمنية عشر", 18);
        IQ.put("ثمنية و عشرة", 18);
        IQ.put("ثمنية و عشره", 18);
        IQ.put("ثمنية وأربعين", 48);
        IQ.put("ثمنية واربعين", 48);
        IQ.put("ثمنية وتسعين", 98);
        IQ.put("ثمنية وثلاثين", 38);
        IQ.put("ثمنية وثمانين", 88);
        IQ.put("ثمنية وثمنين", 88);
        IQ.put("ثمنية وخمسين", 58);
        IQ.put("ثمنية وسبعين", 78);
        IQ.put("ثمنية وستين", 68);
        IQ.put("ثمنية وعشرة", 18);
        IQ.put("ثمنية وعشره", 18);
        IQ.put("ثمنية وعشرين", 28);
        IQ.put("ثمنية وكلاثين", 38);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}