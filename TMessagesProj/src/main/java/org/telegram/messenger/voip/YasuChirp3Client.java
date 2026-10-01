package org.telegram.messenger.voip;

import org.telegram.ui.Components.YasuVoiceTextParser;

import android.util.Log;

import com.google.cloud.speech.v2.AdaptationPhraseSet;
import com.google.cloud.speech.v2.ExplicitDecodingConfig;
import com.google.cloud.speech.v2.Phrase;
import com.google.cloud.speech.v2.PhraseSet;
import com.google.cloud.speech.v2.RecognitionConfig;
import com.google.cloud.speech.v2.RecognitionFeatures;
import com.google.cloud.speech.v2.SpeechGrpc;
import com.google.cloud.speech.v2.StreamingRecognitionConfig;
import com.google.cloud.speech.v2.StreamingRecognitionFeatures;
import com.google.cloud.speech.v2.StreamingRecognitionRequest;
import com.google.cloud.speech.v2.StreamingRecognitionResponse;
import com.google.protobuf.ByteString;

import io.grpc.ManagedChannel;
import io.grpc.Metadata;
import io.grpc.okhttp.OkHttpChannelBuilder;
import io.grpc.stub.MetadataUtils;
import io.grpc.stub.StreamObserver;

public final class YasuChirp3Client {

    private static final String TAG = "YasuChirp3";

    private static final String MODEL = "chirp_3";
    private static final String LANGUAGE = "ar-IQ";

    private static final int SAMPLE_RATE = 16000;
    private static final int CHANNELS = 1;

    private ManagedChannel channel;
    private SpeechGrpc.SpeechStub speechStub;
    private StreamObserver<StreamingRecognitionRequest> requestObserver;

    private String recognizer;

    public interface Listener {
        void onText(String text, boolean isFinal);
        void onError(Throwable error);
        void onComplete();
    }

    public synchronized boolean initialize(
            String host,
            String accessToken,
            String recognizerName
    ) {
        stop();

        if (host == null || host.isEmpty()) {
            Log.e(TAG, "Missing Speech endpoint");
            return false;
        }

        if (accessToken == null || accessToken.isEmpty()) {
            Log.e(TAG, "Missing access token");
            return false;
        }

        if (recognizerName == null || recognizerName.isEmpty()) {
            Log.e(TAG, "Missing recognizer");
            return false;
        }

        recognizer = recognizerName;

        Metadata headers = new Metadata();

        Metadata.Key<String> authorizationKey =
                Metadata.Key.of(
                        "Authorization",
                        Metadata.ASCII_STRING_MARSHALLER
                );

        headers.put(
                authorizationKey,
                "Bearer " + accessToken
        );

        channel = OkHttpChannelBuilder
                .forTarget(host)
                .useTransportSecurity()
                .build();

        speechStub = MetadataUtils.attachHeaders(
                SpeechGrpc.newStub(channel),
                headers
        );

        Log.i(
                TAG,
                "Initialized model=" + MODEL +
                        " language=" + LANGUAGE +
                        " sampleRate=" + SAMPLE_RATE
        );

        return true;
    }

    public synchronized boolean startStream(final Listener listener) {
        if (speechStub == null || requestObserver != null) {
            return false;
        }

        requestObserver = speechStub.streamingRecognize(
                new StreamObserver<StreamingRecognitionResponse>() {

                    @Override
                    public void onNext(
                            StreamingRecognitionResponse response
                    ) {
                        for (
                                com.google.cloud.speech.v2.StreamingRecognitionResult result
                                : response.getResultsList()
                        ) {
                            if (result.getAlternativesCount() == 0) {
                                continue;
                            }

                            String text =
                                    result.getAlternatives(0).getTranscript();

                            if (text == null || text.isEmpty()) {
                                continue;
                            }

                            boolean isFinal = result.getIsFinal();

                            Log.d(
                                    TAG,
                                    "RESULT final=" +
                                            isFinal +
                                            " text=" +
                                            text
                            );

                            if (listener != null) {
                                listener.onText(text, isFinal);
                            }
                        }
                    }

                    @Override
                    public void onError(Throwable error) {
                        synchronized (YasuChirp3Client.this) {
                            requestObserver = null;
                        }

                        Log.e(TAG, "Streaming error", error);

                        if (listener != null) {
                            listener.onError(error);
                        }
                    }

                    @Override
                    public void onCompleted() {
                        synchronized (YasuChirp3Client.this) {
                            requestObserver = null;
                        }

                        Log.i(TAG, "Streaming completed");

                        if (listener != null) {
                            listener.onComplete();
                        }
                    }
                }
        );

        StreamingRecognitionConfig config =
                StreamingRecognitionConfig.newBuilder()
                        .setConfig(buildRecognitionConfig())
                        .setStreamingFeatures(
                                StreamingRecognitionFeatures.newBuilder()
                                        .setInterimResults(true)
                                        .setEnableVoiceActivityEvents(false)
                                        .build()
                        )
                        .build();

        StreamingRecognitionRequest configRequest =
                StreamingRecognitionRequest.newBuilder()
                        .setRecognizer(recognizer)
                        .setStreamingConfig(config)
                        .build();

        requestObserver.onNext(configRequest);

        Log.d(TAG, "Streaming started");

        return true;
    }

    public synchronized boolean sendPcm16(
            byte[] pcm16,
            int offset,
            int length
    ) {
        if (requestObserver == null ||
                pcm16 == null ||
                length <= 0) {
            return false;
        }

        requestObserver.onNext(
                StreamingRecognitionRequest.newBuilder()
                        .setAudio(
                                ByteString.copyFrom(
                                        pcm16,
                                        offset,
                                        length
                                )
                        )
                        .build()
        );

        return true;
    }

    public synchronized void finishStream() {
        if (requestObserver == null) {
            return;
        }

        StreamObserver<StreamingRecognitionRequest> observer =
                requestObserver;

        requestObserver = null;

        observer.onCompleted();

        Log.d(TAG, "Streaming finished");
    }

    public synchronized void stop() {
        if (requestObserver != null) {
            try {
                requestObserver.onCompleted();
            } catch (Throwable ignored) {
            }

            requestObserver = null;
        }

        if (channel != null) {
            try {
                channel.shutdownNow();
            } catch (Throwable ignored) {
            }

            channel = null;
        }

        speechStub = null;
    }

    private static RecognitionConfig buildRecognitionConfig() {
        ExplicitDecodingConfig decoding =
            ExplicitDecodingConfig.newBuilder()
                .setEncoding(
                    ExplicitDecodingConfig.AudioEncoding.LINEAR16
                )
                .setSampleRateHertz(SAMPLE_RATE)
                .setAudioChannelCount(CHANNELS)
                .build();

        RecognitionFeatures features =
            RecognitionFeatures.newBuilder()
                .setEnableAutomaticPunctuation(false)
                .build();

        PhraseSet.Builder phraseSet =
            PhraseSet.newBuilder()
                .setBoost(8.0f);

        String[] numericVocabulary =
            YasuVoiceTextParser.getChirp3NumericVocabulary();

        for (String word : numericVocabulary) {
            if (word == null || word.trim().isEmpty()) {
                continue;
            }

            phraseSet.addPhrases(
                Phrase.newBuilder()
                    .setValue(word)
                    .setBoost(8.0f)
                    .build()
            );
        }

        AdaptationPhraseSet adaptationPhraseSet =
            AdaptationPhraseSet.newBuilder()
                .setInlinePhraseSet(phraseSet.build())
                .build();

        return RecognitionConfig.newBuilder()
            .setModel(MODEL)
            .addLanguageCodes(LANGUAGE)
            .setExplicitDecodingConfig(decoding)
            .setFeatures(features)
            .setAdaptation(
                com.google.cloud.speech.v2.SpeechAdaptation.newBuilder()
                    .addPhraseSets(adaptationPhraseSet)
                    .build()
            )
            .build();
    }
}
