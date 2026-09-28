package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_860 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("ستة", 6);
        IQ.put("ستة عشر", 16);
        IQ.put("ستة و عشرة", 16);
        IQ.put("ستة و عشره", 16);
        IQ.put("ستة وأربعين", 46);
        IQ.put("ستة واربعين", 46);
        IQ.put("ستة وتسعين", 96);
        IQ.put("ستة وثلاثين", 36);
        IQ.put("ستة وثمانين", 86);
        IQ.put("ستة وثمنين", 86);
        IQ.put("ستة وخمسين", 56);
        IQ.put("ستة وسبعين", 76);
        IQ.put("ستة وستين", 66);
        IQ.put("ستة وعشرة", 16);
        IQ.put("ستة وعشره", 16);
        IQ.put("ستة وعشرين", 26);
        IQ.put("ستة وكلاثين", 36);
        FU.put("ستة", 6);
        FU.put("ستة عشر", 16);
        FU.put("ستة و عشرة", 16);
        FU.put("ستة و عشره", 16);
        FU.put("ستة وأربعون", 46);
        FU.put("ستة وأربعين", 46);
        FU.put("ستة واربعون", 46);
        FU.put("ستة واربعين", 46);
        FU.put("ستة وتسعون", 96);
        FU.put("ستة وتسعين", 96);
        FU.put("ستة وثلاثون", 36);
        FU.put("ستة وثلاثين", 36);
        FU.put("ستة وثمانون", 86);
        FU.put("ستة وثمانين", 86);
        FU.put("ستة وخمسون", 56);
        FU.put("ستة وخمسين", 56);
        FU.put("ستة وسبعون", 76);
        FU.put("ستة وسبعين", 76);
        FU.put("ستة وستون", 66);
        FU.put("ستة وستين", 66);
        FU.put("ستة وعشرة", 16);
        FU.put("ستة وعشره", 16);
        FU.put("ستة وعشرون", 26);
        FU.put("ستة وعشرين", 26);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}