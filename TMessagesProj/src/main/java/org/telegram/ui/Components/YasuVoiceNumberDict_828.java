package org.telegram.ui.Components;

import java.util.HashMap;
import java.util.Map;

final class YasuVoiceNumberDict_828 {
    private static final Map<String, Integer> IQ = new HashMap<>();
    private static final Map<String, Integer> FU = new HashMap<>();
    static {
        IQ.put("سبعه", 7);
        IQ.put("سبعه عشر", 17);
        IQ.put("سبعه و عشرة", 17);
        IQ.put("سبعه و عشره", 17);
        IQ.put("سبعه وأربعين", 47);
        IQ.put("سبعه واربعين", 47);
        IQ.put("سبعه وتسعين", 97);
        IQ.put("سبعه وثلاثين", 37);
        IQ.put("سبعه وثمانين", 87);
        IQ.put("سبعه وثمنين", 87);
        IQ.put("سبعه وخمسين", 57);
        IQ.put("سبعه وسبعين", 77);
        IQ.put("سبعه وستين", 67);
        IQ.put("سبعه وعشرة", 17);
        IQ.put("سبعه وعشره", 17);
        IQ.put("سبعه وعشرين", 27);
        IQ.put("سبعه وكلاثين", 37);
        FU.put("سبعه", 7);
        FU.put("سبعه عشر", 17);
        FU.put("سبعه و عشرة", 17);
        FU.put("سبعه و عشره", 17);
        FU.put("سبعه وأربعون", 47);
        FU.put("سبعه وأربعين", 47);
        FU.put("سبعه واربعون", 47);
        FU.put("سبعه واربعين", 47);
        FU.put("سبعه وتسعون", 97);
        FU.put("سبعه وتسعين", 97);
        FU.put("سبعه وثلاثون", 37);
        FU.put("سبعه وثلاثين", 37);
        FU.put("سبعه وثمانون", 87);
        FU.put("سبعه وثمانين", 87);
        FU.put("سبعه وخمسون", 57);
        FU.put("سبعه وخمسين", 57);
        FU.put("سبعه وسبعون", 77);
        FU.put("سبعه وسبعين", 77);
        FU.put("سبعه وستون", 67);
        FU.put("سبعه وستين", 67);
        FU.put("سبعه وعشرة", 17);
        FU.put("سبعه وعشره", 17);
        FU.put("سبعه وعشرون", 27);
        FU.put("سبعه وعشرين", 27);
    }
    static Integer iq(String s) { return IQ.get(s); }
    static Integer fu(String s) { return FU.get(s); }
}