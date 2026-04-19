#include "../headers/mapdetector.h"
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <QString>
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QStandardPaths>
#include <QCoreApplication>

static QString layoutPathForName(const QString& name) {
    if (name == "abyss")    return "map_layouts/Abyss_layout.png";
    if (name == "ascent")   return "map_layouts/Ascent_layout.png";
    if (name == "bind")     return "map_layouts/Bind_layout.png";
    if (name == "breeze")   return "map_layouts/Breeze_layout.png";
    if (name == "fracture") return "map_layouts/Fracture_layout.png";
    if (name == "haven")    return "map_layouts/Haven_layout.png";
    if (name == "icebox")   return "map_layouts/Icebox_layout.png";
    if (name == "lotus")    return "map_layouts/Lotus_layout.png";
    if (name == "pearl")    return "map_layouts/Pearl_layout.png";
    if (name == "split")    return "map_layouts/Split_layout.png";
    if (name == "sunset")   return "map_layouts/Sunset_layout.png";
    return "";
}

// Class index -> map name (matches training order from export)
static QString yoloClassToMap(int idx) {
    switch (idx) {
    case 0: return "sunset";
    case 1: return "lotus";
    case 2: return "pearl";
    case 3: return "fracture";
    case 4: return "breeze";
    case 5: return "icebox";
    case 6: return "bind";
    case 7: return "haven";
    case 8: return "split";
    case 9: return "ascent";
    default: return "";
    }
}

// Layer 2: YOLO via OpenCV DNN - no extra libraries needed
// Samples multiple frames and accumulates scores so short detections
// don't win over consistent ones.
static QString detectMapWithYOLO(const std::string& videoPath,
    double maxSeconds = 600.0,
    int    maxFrames = 20)
{
    // Load model once - path is relative to the exe
    static cv::dnn::Net net;
    static bool loaded = false;
    static bool failed = false;

    if (failed) return "";

    if (!loaded) {
        QString modelPath = QCoreApplication::applicationDirPath() + "/models/map_detection.onnx";
        try {
            net = cv::dnn::readNetFromONNX(modelPath.toStdString());
            net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
            loaded = true;
            qDebug() << "[MapDetector][YOLO] Model loaded from:" << modelPath;
        }
        catch (const cv::Exception& e) {
            qDebug() << "[MapDetector][YOLO] Could not load model:" << e.what();
            qDebug() << "[MapDetector][YOLO] Place map_detection.onnx in the models/ folder next to the exe.";
            failed = true;
            return "";
        }
    }

    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) return "";

    double fps = cap.get(cv::CAP_PROP_FPS);
    if (fps <= 0) fps = 30.0;
    double totalSec = cap.get(cv::CAP_PROP_FRAME_COUNT) / fps;
    double scanEnd = std::min(totalSec, maxSeconds);
    double step = scanEnd / std::max(maxFrames, 1);

    const int NUM_CLASSES = 10;
    std::vector<float> scores(NUM_CLASSES, 0.f);
    int samplesUsed = 0;

    for (int i = 0; i < maxFrames; ++i) {
        double seekSec = 10.0 + i * step;
        if (seekSec > scanEnd) break;

        cap.set(cv::CAP_PROP_POS_FRAMES, seekSec * fps);
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) continue;

        // Crop the minimap region (top-left of frame)
        int x = (int)(frame.cols * 0.024);
        int y = (int)(frame.rows * 0.083);
        int w = (int)(frame.cols * 0.160);
        int h = (int)(frame.rows * 0.347);
        x = std::max(0, std::min(x, frame.cols - 1));
        y = std::max(0, std::min(y, frame.rows - 1));
        w = std::min(w, frame.cols - x);
        h = std::min(h, frame.rows - y);
        if (w <= 0 || h <= 0) continue;

        cv::Mat minimap = frame(cv::Rect(x, y, w, h));

        // Prepare blob for YOLO
        cv::Mat blob;
        cv::dnn::blobFromImage(minimap, blob, 1.0 / 255.0,
            cv::Size(640, 640), cv::Scalar(), true, false);
        net.setInput(blob);

        std::vector<cv::Mat> outputs;
        net.forward(outputs, net.getUnconnectedOutLayersNames());
        if (outputs.empty() || outputs[0].empty()) continue;

        // YOLOv8 output: [1, 14, 8400] -> transpose to [8400, 14]
        cv::Mat out = outputs[0];
        cv::Mat data(out.size[1], out.size[2], CV_32F, out.ptr<float>());
        cv::transpose(data, data); // now [8400, 14]

        // Accumulate best score per class across all detections
        for (int d = 0; d < data.rows; ++d) {
            float* row = data.ptr<float>(d);
            for (int c = 0; c < NUM_CLASSES; ++c) {
                float s = row[4 + c];
                if (s > 0.25f)
                    scores[c] += s;
            }
        }
        ++samplesUsed;
    }
    cap.release();

    if (samplesUsed == 0) {
        qDebug() << "[MapDetector][YOLO] No frames sampled.";
        return "";
    }

    int   bestIdx = (int)(std::max_element(scores.begin(), scores.end()) - scores.begin());
    float bestScore = scores[bestIdx];

    if (bestScore <= 0.f) {
        qDebug() << "[MapDetector][YOLO] No confident detections.";
        return "";
    }

    QString bestMap = yoloClassToMap(bestIdx);
    qDebug() << "[MapDetector][YOLO] Detected:" << bestMap
        << "accumulated score:" << bestScore;
    return bestMap;
}

// Layer 1: OCR - reads "CURRENT: HAVEN" from the broadcast overlay bar
static QString detectMapWithOCR(cv::VideoCapture& cap, double fps, double maxSeek) {
    QStringList knownMaps = {
        "abyss","ascent","bind","breeze","fracture",
        "haven","icebox","lotus","pearl","split","sunset"
    };

    for (double seekSec = 10.0; seekSec <= maxSeek; seekSec += 30.0) {
        cap.set(cv::CAP_PROP_POS_FRAMES, seekSec * fps);
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) continue;

        qDebug() << "[MapDetector][OCR] Checking frame at" << (int)seekSec << "seconds...";

        double sx = frame.cols / 1280.0;
        double sy = frame.rows / 720.0;
        int cropW = std::min((int)(400 * sx), frame.cols);
        int cropH = std::min((int)(22 * sy), frame.rows);
        cv::Mat bar = frame(cv::Rect(0, 0, cropW, cropH));

        cv::Mat upscaled;
        cv::resize(bar, upscaled, cv::Size(), 3, 3, cv::INTER_CUBIC);

        QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString tempImage = tempDir + "/vv_bar.png";
        QString tempText = tempDir + "/vv_bar";
        cv::imwrite(tempImage.toStdString(), upscaled);

        QProcess process;
        process.start("tesseract", {
            tempImage, tempText,
            "--psm", "7",
            "-c", "tessedit_char_whitelist=ABCDEFGHIJKLMNOPQRSTUVWXYZ: "
            });
        process.waitForFinished(5000);

        QFile file(tempText + ".txt");
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) continue;
        QString text = file.readAll().toUpper().simplified();
        file.close();

        qDebug() << "[MapDetector][OCR] Read:" << text;

        int idx = text.indexOf("CURRENT");
        if (idx >= 0) {
            QString after = text.mid(idx + 7).simplified();
            if (after.startsWith(":")) after = after.mid(1).simplified();
            QString firstWord = after.split(" ").first().toLower();
            if (knownMaps.contains(firstWord)) {
                qDebug() << "[MapDetector][OCR] Detected:" << firstWord;
                return firstWord;
            }
        }
        for (const QString& map : knownMaps)
            if (text.contains(map.toUpper())) {
                qDebug() << "[MapDetector][OCR] Detected:" << map;
                return map;
            }
    }
    return "";
}

// Public entry point
QString detectMapFromVideo(const std::string& videoPath) {

    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        qDebug() << "[MapDetector] Could not open video.";
        return "";
    }

    double fps = cap.get(cv::CAP_PROP_FPS);
    if (fps <= 0) fps = 30.0;
    double totalSeconds = cap.get(cv::CAP_PROP_FRAME_COUNT) / fps;
    double maxSeek = std::min(totalSeconds, 600.0);
    qDebug() << "[MapDetector] Video length:" << (int)totalSeconds << "seconds";

    // Layer 1: OCR
    QString result = detectMapWithOCR(cap, fps, maxSeek);
    cap.release();

    if (!result.isEmpty())
        return layoutPathForName(result);

    // Layer 2: YOLO fallback
    qDebug() << "[MapDetector] OCR failed. Trying YOLO...";
    result = detectMapWithYOLO(videoPath);

    if (!result.isEmpty())
        return layoutPathForName(result);

    qDebug() << "[MapDetector] Could not detect map.";
    return "";
}

void saveMapNameReference(const std::string& videoPath, const std::string& mapName) {
    Q_UNUSED(videoPath);
    Q_UNUSED(mapName);
}