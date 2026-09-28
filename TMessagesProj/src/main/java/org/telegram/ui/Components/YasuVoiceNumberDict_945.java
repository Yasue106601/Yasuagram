package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_945 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("سته", 6);
        IQ.put("سته عشر", 16);
        IQ.put("سته و عشرة", 16);
        IQ.put("سته و عشره", 16);
        IQ.put("سته وأربعين", 46);
        IQ.put("سته واربعين", 46);
        IQ.put("سته وتسعين", 96);
        IQ.put("سته وثلاثين", 36);
        IQ.put("سته وثمانين", 86);
        IQ.put("سته وثمنين", 86);
        IQ.put("سته وخمسين", 56);
        IQ.put("سته وسبعين", 76);
        IQ.put("سته وستين", 66);
        IQ.put("سته وعشرة", 16);
        IQ.put("سته وعشره", 16);
        IQ.put("سته وعشرين", 26);
        IQ.put("سته وكلاثين", 36);
        FU.put("سته", 6);
        FU.put("سته عشر", 16);
        FU.put("سته و عشرة", 16);
        FU.put("سته و عشره", 16);
        FU.put("سته وأربعون", 46);
        FU.put("سته وأربعين", 46);
        FU.put("سته واربعون", 46);
        FU.put("سته واربعين", 46);
        FU.put("سته وتسعون", 96);
        FU.put("سته وتسعين", 96);
        FU.put("سته وثلاثون", 36);
        FU.put("سته وثلاثين", 36);
        FU.put("سته وثمانون", 86);
        FU.put("سته وثمانين", 86);
        FU.put("سته وخمسون", 56);
        FU.put("سته وخمسين", 56);
        FU.put("سته وسبعون", 76);
        FU.put("سته وسبعين", 76);
        FU.put("سته وستون", 66);
        FU.put("سته وستين", 66);
        FU.put("سته وعشرة", 16);
        FU.put("سته وعشره", 16);
        FU.put("سته وعشرون", 26);
        FU.put("سته وعشرين", 26);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}