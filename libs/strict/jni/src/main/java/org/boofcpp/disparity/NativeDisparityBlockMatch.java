package org.boofcpp.disparity;

import boofcv.abst.disparity.StereoDisparityBlockMatch;
import boofcv.alg.InputSanityCheck;
import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.struct.image.GrayF32;
import boofcv.struct.image.GrayU8;
import boofcv.struct.image.ImageGray;
import boofcv.struct.image.ImageType;
import org.boofcpp.BoofCPPStrict;

/// Native stereo block matching, a replacement for BoofCV's WrapBaseBlockMatch. Use [#isSupported] to see
/// if a configuration can be computed natively.
///
/// BOOFCPP_OVERRIDE BOverrideStereoDisparity.opCreateBlockMatch
public class NativeDisparityBlockMatch<T extends ImageGray<T>, DI extends ImageGray<DI>>
        implements StereoDisparityBlockMatch<T, DI> {
    static {
        BoofCPPStrict.loadlib();
    }

    protected long nativePtr;

    final ImageType<T> inputType;
    final Class<DI> disparityType;
    final int disparityMin;
    final int disparityRange;
    final int radiusY;

    DI disparity;
    GrayF32 score;

    public NativeDisparityBlockMatch( ConfigDisparityBM config, Class<T> imageType, Class<DI> disparityType ) {
        if (!isSupported(config, imageType))
            throw new IllegalArgumentException("Configuration not supported natively");
        config.checkValidity();
        if (config.subpixel != (disparityType == GrayF32.class))
            throw new IllegalArgumentException("Disparity type must be GrayF32 with subpixel and GrayU8 without");

        this.inputType = ImageType.single(imageType);
        this.disparityType = disparityType;
        this.disparityMin = config.disparityMin;
        this.disparityRange = config.disparityRange;
        this.radiusY = config.regionRadiusY;
        nativeinit(config, imageType == GrayF32.class);
        setScoreEnabled(config.saveScore);
    }

    /// Native block matching, or null if the configuration and image type can't be computed natively, which makes
    /// BoofCV use its Java implementation. BoofCPPStrict.initialize() registers it with
    /// BOverrideStereoDisparity.opCreateBlockMatch
    public static <T extends ImageGray<T>, DI extends ImageGray<DI>> StereoDisparityBlockMatch<T, DI>
    create( ConfigDisparityBM config, Class<T> imageType, Class<DI> disparityType ) {
        if (!isSupported(config, imageType))
            return null;
        return new NativeDisparityBlockMatch<>(config, imageType, disparityType);
    }

    /// True if the configuration and image type can be computed natively. Every error type is supported with
    /// GrayU8 and GrayF32 images.
    public static boolean isSupported( ConfigDisparityBM config, Class<?> imageType ) {
        return imageType == GrayU8.class || imageType == GrayF32.class;
    }

    @Override public void process( T imageLeft, T imageRight ) {
        InputSanityCheck.checkSameShape(imageLeft, imageRight);
        if (disparity == null || disparity.width != imageLeft.width || disparity.height != imageLeft.height) {
            // make sure the image borders are marked as invalid
            disparity = GeneralizedImageOps.createSingleBand(disparityType, imageLeft.width, imageLeft.height);
            GImageMiscOps.fill(disparity, getInvalidValue());
        }
        disparity.reshape(imageLeft);
        if (score != null)
            score.reshape(disparity);
        nativeprocess(imageLeft, imageRight, disparity, score);
    }

    @Override public void setScoreEnabled( boolean enabled ) {
        if (enabled && score != null)
            return;
        score = enabled ? new GrayF32(1, 1) : null;
    }

    /// Returns this, since the implementation is native
    @Override public <Obj> Obj getAlg() {
        return (Obj)this;
    }

    @Override public DI getDisparity() {return disparity;}

    @Override public GrayF32 getDisparityScore() {return score;}

    @Override public int getDisparityMin() {return disparityMin;}

    @Override public int getDisparityRange() {return disparityRange;}

    @Override public int getInvalidValue() {return disparityRange;}

    @Override public int getBorderX() {return 0;}

    @Override public int getBorderY() {return radiusY;}

    @Override public ImageType<T> getInputType() {return inputType;}

    @Override public Class<DI> getDisparityType() {return disparityType;}

    @Override protected void finalize() throws Throwable {
        nativedestroy();
        super.finalize();
    }

    // Protected so a variant's subclass replaces them with its native code
    protected native void nativeinit( ConfigDisparityBM config, boolean imageF32 );

    protected native void nativedestroy();

    protected native void nativeprocess( T left, T right, DI disparity, GrayF32 score );
}
