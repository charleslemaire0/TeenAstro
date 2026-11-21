// JSON front end for the StellarSolver library.
// https://github.com/rlancaste/stellarsolver
//
// The photo is solved by StellarSolver's own star extractor and its internal
// Astrometry.net build. Star x,y are pixels of this image, origin at the
// top left, y downward. stdout is one JSON object and nothing else.
//
//   stellarsolver-json photo.jpg index-folder scale-low scale-high downsample seconds
//
// scale-low and scale-high are the image width in degrees.

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointF>
#include <QTextStream>

#include "ssolverutils/fileio.h"
#include "stellarsolver.h"

namespace {

void write_object(const QJsonObject &object) {
    QTextStream out(stdout);
    out << QJsonDocument(object).toJson(QJsonDocument::Compact);
    out.flush();
}

void fail(const QString &reason) {
    QJsonObject object;
    object.insert(QStringLiteral("ok"), false);
    object.insert(QStringLiteral("reason"), reason);
    write_object(object);
}

}  // namespace

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    const QStringList args = QCoreApplication::arguments();
    if (args.size() != 7) {
        fail(QStringLiteral(
            "StellarSolver needs the photo, the index folder, the scale range, the downsample, and the time limit."));
        return 0;
    }

    bool ok_low = false;
    bool ok_high = false;
    bool ok_down = false;
    bool ok_seconds = false;
    const QString image = args.at(1);
    const QString index = args.at(2);
    const double low = args.at(3).toDouble(&ok_low);
    const double high = args.at(4).toDouble(&ok_high);
    const int downsample = args.at(5).toInt(&ok_down);
    const int seconds = args.at(6).toInt(&ok_seconds);
    if (!ok_low || !ok_high || !ok_down || !ok_seconds || low <= 0 || high <= low ||
        downsample < 1 || seconds < 1) {
        fail(QStringLiteral("The scale, downsample, or time limit is not usable."));
        return 0;
    }

    fileio image_loader;
    image_loader.logToSignal = false;
    if (!image_loader.loadImage(image)) {
        fail(QStringLiteral("The photo could not be read."));
        return 0;
    }

    StellarSolver solver(image_loader.getStats(), image_loader.getImageBuffer());
    solver.setIndexFolderPaths(QStringList() << index);
    solver.setParameterProfile(SSolver::Parameters::PARALLEL_LARGESCALE);
    SSolver::Parameters params = solver.getCurrentParameters();
    params.autoDownsample = false;
    params.downsample = downsample;
    params.solverTimeLimit = seconds;
    solver.setParameters(params);
    solver.setSearchScale(low, high, SSolver::DEG_WIDTH);
    solver.setLogLevel(SSolver::LOG_NONE);

    QElapsedTimer timer;
    timer.start();
    const bool solved = solver.solve();
    const double elapsed_ms = static_cast<double>(timer.elapsed());
    if (!solved) {
        fail(QStringLiteral("StellarSolver found no match in this photo."));
        return 0;
    }

    const FITSImage::Solution solution = solver.getSolution();
    QJsonArray stars;
    const QList<FITSImage::Star> extracted = solver.getStarList();
    const QList<FITSImage::Star> &found =
        extracted.isEmpty() ? solver.getStarListFromSolve() : extracted;
    for (const FITSImage::Star &star : found) {
        // Star x,y are on the downsampled image. The WCS expects full-image pixels.
        const double full_x = static_cast<double>(star.x) * downsample;
        const double full_y = static_cast<double>(star.y) * downsample;
        float ra = 0.f;
        float dec = 0.f;
        FITSImage::wcs_point sky;
        if (solver.pixelToWCS(QPointF(full_x, full_y), sky)) {
            ra = sky.ra;
            dec = sky.dec;
        }
        QJsonObject row;
        row.insert(QStringLiteral("x"), full_x);
        row.insert(QStringLiteral("y"), full_y);
        if (!(ra == 0.f && dec == 0.f)) {
            row.insert(QStringLiteral("ra_deg"), static_cast<double>(ra));
            row.insert(QStringLiteral("dec_deg"), static_cast<double>(dec));
        }
        stars.append(row);
    }

    QJsonObject object;
    object.insert(QStringLiteral("ok"), true);
    object.insert(QStringLiteral("ra_deg"), solution.ra);
    object.insert(QStringLiteral("dec_deg"), solution.dec);
    object.insert(QStringLiteral("roll_deg"), solution.orientation);
    object.insert(QStringLiteral("ms"), elapsed_ms);
    object.insert(QStringLiteral("epoch_equinox"), 2000);
    object.insert(QStringLiteral("stars"), stars);
    write_object(object);
    return 0;
}
