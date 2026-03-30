#include "../headers/mapdetector.h"
#include <opencv2/opencv.hpp>
#include <QString>
#include <QDebug>
#include <QProcess>
#include <QFile>
#include <QStandardPaths>

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

QString detectMapFromVideo(const std::string& videoPath) {

    cv::VideoCapture cap(videoPath);
    if (!cap.isOpened()) {
        qDebug() << "[MapDetector] Could not open video.";
        return "";
    }

    double fps = cap.get(cv::CAP_PROP_FPS);
    if (fps <= 0) fps = 30.0;

    cap.set(cv::CAP_PROP_POS_FRAMES, 20.0 * fps);
    cv::Mat frame;
    cap >> frame;
    cap.release();

    if (frame.empty()) {
        qDebug() << "[MapDetector] Could not read frame.";
        return "";
    }

    qDebug() << "[MapDetector] Frame size:" << frame.cols << "x" << frame.rows;

    // ── Crop the broadcast bar ────────────────────────────────────────────
    // The full top bar contains "CURRENT: HAVEN  NEXT: BREEZE  DECIDER: ..."
    // We crop a wider strip to give Tesseract more context to read from.
    double sx = frame.cols / 1280.0;
    double sy = frame.rows / 720.0;

    int cropW = std::min((int)(400 * sx), frame.cols);
    int cropH = std::min((int)(22 * sy), frame.rows);
    cv::Mat bar = frame(cv::Rect(0, 0, cropW, cropH));

    // Upscale 3x - Tesseract works much better on larger text
    cv::Mat upscaled;
    cv::resize(bar, upscaled, cv::Size(), 3, 3, cv::INTER_CUBIC);

    // Save to a temp file for Tesseract to read
    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    QString tempImage = tempDir + "/vv_bar.png";
    QString tempText = tempDir + "/vv_bar";
    cv::imwrite(tempImage.toStdString(), upscaled);

    // ── Run Tesseract ─────────────────────────────────────────────────────
    QProcess process;
    process.start("tesseract", {
        tempImage,         // input image
        tempText,          // output file base (tesseract adds .txt)
        "--psm", "7",      // treat image as a single line of text
        "-c", "tessedit_char_whitelist=ABCDEFGHIJKLMNOPQRSTUVWXYZ: "
        });
    process.waitForFinished(5000);

    // Read the output text file Tesseract created
    QString outputFile = tempText + ".txt";
    QFile file(outputFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "[MapDetector] Could not read Tesseract output.";
        return "";
    }
    QString text = file.readAll().toUpper().simplified();
    file.close();

    qDebug() << "[MapDetector] Tesseract read:" << text;

    // ── Find map name after "CURRENT:" ───────────────────────────────────
    QStringList knownMaps = {
        "abyss","ascent","bind","breeze","fracture",
        "haven","icebox","lotus","pearl","split","sunset"
    };

    // Look for "CURRENT" in the text then check the word after it
    int idx = text.indexOf("CURRENT");
    if (idx >= 0) {
        // Get everything after "CURRENT:"
        QString after = text.mid(idx + 7).simplified();
        // Remove the colon if present
        if (after.startsWith(":")) after = after.mid(1).simplified();
        // First word should be the map name
        QString firstWord = after.split(" ").first().toLower();
        qDebug() << "[MapDetector] Candidate map name:" << firstWord;

        if (knownMaps.contains(firstWord))
            return layoutPathForName(firstWord);
    }

    // Fallback: just search the whole text for any known map name
    for (const QString& map : knownMaps) {
        if (text.contains(map.toUpper())) {
            qDebug() << "[MapDetector] Found map name in text:" << map;
            return layoutPathForName(map);
        }
    }

    qDebug() << "[MapDetector] No map name found in OCR output.";
    return "";
}

void saveMapNameReference(const std::string& videoPath, const std::string& mapName) {
    // Not needed anymore with OCR - kept for compatibility
    Q_UNUSED(videoPath);
    Q_UNUSED(mapName);
}