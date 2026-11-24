package org.boofcpp.testing;

import boofcv.alg.misc.GImageMiscOps;
import boofcv.core.image.GeneralizedImageOps;
import boofcv.core.image.border.FactoryImageBorder;
import boofcv.override.BOverrideManager;
import boofcv.struct.ConfigLength;
import boofcv.struct.border.BorderType;
import boofcv.struct.border.ImageBorder;
import boofcv.struct.convolve.Kernel1D;
import boofcv.struct.convolve.Kernel1D_F32;
import boofcv.struct.convolve.Kernel1D_F64;
import boofcv.struct.convolve.Kernel1D_S32;
import boofcv.struct.convolve.Kernel2D;
import boofcv.struct.convolve.Kernel2D_F32;
import boofcv.struct.convolve.Kernel2D_F64;
import boofcv.struct.convolve.Kernel2D_S32;
import boofcv.struct.image.*;
import georegression.struct.point.Point2D_I32;
import org.ddogleg.struct.DogArray;
import org.ddogleg.struct.FastAccess;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;

import java.io.File;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Function;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.fail;

/// The contract of every hook a variant registers with BoofCV: each function either finishes or throws a
/// RuntimeException, which makes BoofCV use its Java code. Anything else, e.g. an UnsatisfiedLinkError from a missing
/// native function, would reach the user. Every function of the hooks, and of the objects hooks create, is called with
/// small images of every gray type. Their results aren't checked. Arguments are chosen by their type, see
/// [#argument], unless the function is in [#customArguments]
@SuppressWarnings({"rawtypes", "unchecked"})
public abstract class HookContractChecks extends BoofCPPStandardJUnit {

    /// Arguments for hooks whose functions need values [#argument] doesn't give, keyed by "BOverride class.field".
    /// Takes the image type
    protected final Map<String, Function<Class, Object[]>> customArguments = new HashMap<>();

    protected HookContractChecks() {
        // Pixel values must be within [minValue, maxValue]
        customArguments.put("BOverrideFactoryThresholdBinary.globalOtsu",
                type -> new Object[]{0.0, 255.0, 1.0, false, type});
        customArguments.put("BOverrideFactoryThresholdBinary.globalEntropy",
                type -> new Object[]{0, 255, 1.0, false, type});
    }

    /// Registers the variant's hooks with BoofCV
    protected abstract void registerHooks();

    @AfterEach
    public void clearHooks() {
        BOverrideManager.clear();
    }

    @Test public void hooksKeepContract() throws Exception {
        BOverrideManager.clear();
        registerHooks();

        List<String> failures = new ArrayList<>();
        int hooks = 0;
        for (Field field : hookFields()) {
            Object hook = field.get(null);
            if (hook == null)
                continue;
            hooks++;
            String name = field.getDeclaringClass().getSimpleName() + "." + field.getName();
            Function<Class, Object[]> custom = customArguments.get(name);
            for (Class type : BoofCPPTesting.GRAY_TYPES) {
                for (Method method : interfaceMethods(hook.getClass())) {
                    Object[] args = custom != null ? custom.apply(type) : arguments(method, type);
                    Object created = call(hook, method, args, type, name, failures);
                    // Objects a hook creates, e.g. a thresholding algorithm
                    if (created == null || method.getReturnType().isPrimitive())
                        continue;
                    for (Method m : publicMethods(method.getReturnType())) {
                        call(created, m, arguments(m, type), type, name + "." + method.getName(), failures);
                    }
                }
            }
        }
        assertFalse(hooks == 0, "No hooks were registered");
        if (!failures.isEmpty())
            fail(String.join("\n", failures));
    }

    /// Every class in the variant's main sources that implements a BOverride interface has a hook tag, see
    /// [HookTags]. After [#registerHooks()], each
    /// BOOFCPP_OVERRIDE field is claimed by one class and registered, and holds an instance of the class, or creates
    /// one if it's a factory; every BOverride interface the class implements has a tagged field; a BOOFCPP_IGNORE class
    /// isn't registered
    @Test public void hookTagsMatchRegistration() throws Exception {
        BOverrideManager.clear();
        registerHooks();

        List<String> failures = new ArrayList<>();
        Map<String, String> claimed = new HashMap<>();
        for (HookTags tags : HookTags.load(new File("src/main/java"))) {
            String name = tags.className;
            if (tags.interfaces.isEmpty() && tags.overrides.isEmpty() && !tags.ignore)
                continue;
            if (tags.overrides.isEmpty() && !tags.ignore) {
                failures.add(name + " implements " + tags.interfaces + " but has no BOOFCPP_OVERRIDE or BOOFCPP_IGNORE");
                continue;
            }
            if (tags.ignore && !tags.overrides.isEmpty()) {
                failures.add(name + " has both BOOFCPP_OVERRIDE and BOOFCPP_IGNORE");
                continue;
            }
            Class type = Class.forName(name);

            List<String> taggedTypes = new ArrayList<>();
            for (String override : tags.overrides) {
                Field field = hookField(override);
                if (field == null) {
                    failures.add(name + ": " + override + " isn't a hook field of a BOverride class BoofCV knows");
                    continue;
                }
                taggedTypes.add(interfaceName(field.getType()));
                String other = claimed.put(override, name);
                if (other != null)
                    failures.add(name + " and " + other + " both claim " + override);
                Object hook = field.get(null);
                if (hook == null) {
                    failures.add(name + ": " + override + " isn't registered");
                } else if (field.getType().isAssignableFrom(type)) {
                    if (!type.isInstance(hook))
                        failures.add(name + ": " + override + " holds a " + hook.getClass().getName());
                } else if (!createdClasses(field).stream().anyMatch(type::isAssignableFrom)) {
                    // A factory hook, e.g. a constructor reference, which creates the tagged class
                    failures.add(name + ": " + override + " creates " + createdClasses(field));
                }
            }
            for (String implemented : tags.interfaces) {
                if (!taggedTypes.contains(implemented))
                    failures.add(name + " implements " + implemented + " but no field of that type is tagged");
            }
            if (tags.ignore) {
                for (Field field : hookFields()) {
                    if (type.isInstance(field.get(null)))
                        failures.add(name + " has BOOFCPP_IGNORE but is registered with " + field.getName());
                }
            }
        }
        if (!failures.isEmpty())
            fail(String.join("\n", failures));
    }

    /// Classes of the objects a registered hook creates, calling each of its functions for every gray image type
    private List<Class> createdClasses( Field field ) throws IllegalAccessException {
        Object hook = field.get(null);
        String name = field.getDeclaringClass().getSimpleName() + "." + field.getName();
        Function<Class, Object[]> custom = customArguments.get(name);
        List<Class> created = new ArrayList<>();
        for (Class type : BoofCPPTesting.GRAY_TYPES) {
            for (Method method : interfaceMethods(hook.getClass())) {
                Object[] args = custom != null ? custom.apply(type) : arguments(method, type);
                Object result = call(hook, method, args, type, name, new ArrayList<>());
                if (result != null && !created.contains(result.getClass()))
                    created.add(result.getClass());
            }
        }
        return created;
    }

    /// The hook field named "BOverride class.field", or null if BoofCV doesn't know it
    private static Field hookField( String name ) {
        String[] parts = name.split("\\.");
        if (parts.length != 2)
            return null;
        for (Field field : hookFields()) {
            if (field.getDeclaringClass().getSimpleName().equals(parts[0]) && field.getName().equals(parts[1]))
                return field;
        }
        return null;
    }

    /// "BOverride class.interface", the way a class declaration names it
    private static String interfaceName( Class type ) {
        Class enclosing = type.getEnclosingClass();
        return (enclosing == null ? "" : enclosing.getSimpleName() + ".") + type.getSimpleName();
    }

    /// Calls the function and records it if it throws anything other than a RuntimeException. Returns its result
    private Object call( Object target, Method method, Object[] args, Class type, String hook, List<String> failures ) {
        try {
            method.setAccessible(true);
            return method.invoke(target, args);
        } catch (InvocationTargetException e) {
            if (!(e.getCause() instanceof RuntimeException))
                failures.add(hook + " " + method.getName() + " " + type.getSimpleName() + ": " + e.getCause());
        } catch (IllegalAccessException | IllegalArgumentException e) {
            failures.add(hook + " " + method.getName() + " " + type.getSimpleName() + " can't be called: " + e);
        }
        return null;
    }

    /// An argument for each of the function's parameters
    private Object[] arguments( Method method, Class type ) {
        Object[] args = new Object[method.getParameterCount()];
        for (int i = 0; i < args.length; i++) {
            args[i] = argument(method.getParameterTypes()[i], type);
        }
        return args;
    }

    /// A valid looking argument of the parameter's type for images of 'type'
    private Object argument( Class param, Class type ) {
        if (param == int.class || param == long.class)
            return param == int.class ? (Object)1 : (Object)1L;
        if (param == double.class || param == float.class)
            return param == double.class ? (Object)1.0 : (Object)1.0f;
        if (param == boolean.class)
            return false;
        if (param == Class.class)
            return type;
        if (param.isEnum())
            return param.getEnumConstants()[0];
        if (ImageBase.class.isAssignableFrom(param)) {
            Class imageType = param.isAssignableFrom(type) ? type : param;
            ImageBase image = GeneralizedImageOps.createImage(imageType, 12, 10, 2);
            GImageMiscOps.fillUniform(image, rand, 0, 100);
            return image;
        }
        // Points, e.g. census sample points
        if (param == FastAccess.class) {
            var points = new DogArray<>(Point2D_I32::new);
            points.grow().setTo(1, 0);
            points.grow().setTo(0, -1);
            return points;
        }
        boolean f32 = type == GrayF32.class, f64 = type == GrayF64.class;
        if (param == Kernel1D.class)
            return f32 ? new Kernel1D_F32(3, new float[]{1, 2, 1}) :
                    f64 ? new Kernel1D_F64(3, new double[]{1, 2, 1}) : new Kernel1D_S32(3, new int[]{1, 2, 1});
        if (param == Kernel2D.class)
            return f32 ? new Kernel2D_F32(3, new float[]{1, 2, 1, 2, 4, 2, 1, 2, 1}) :
                    f64 ? new Kernel2D_F64(3, new double[]{1, 2, 1, 2, 4, 2, 1, 2, 1}) :
                            new Kernel2D_S32(3, new int[]{1, 2, 1, 2, 4, 2, 1, 2, 1});
        if (param == ImageBorder.class)
            return FactoryImageBorder.single(BorderType.EXTENDED, type);
        if (param == ConfigLength.class)
            return ConfigLength.fixed(3);
        try {
            return param.getConstructor().newInstance();
        } catch (ReflectiveOperationException | RuntimeException e) {
            return null;
        }
    }

    /// Public static fields of every BOverride class BoofCV knows about, which hold the hooks
    private static List<Field> hookFields() {
        List<Field> fields = new ArrayList<>();
        for (Class c : new ArrayList<>(BOverrideManager.list)) {
            for (Field field : c.getFields()) {
                int mod = field.getModifiers();
                if (Modifier.isStatic(mod) && !Modifier.isFinal(mod) && !field.getType().isPrimitive())
                    fields.add(field);
            }
        }
        return fields;
    }

    /// Functions of the BOverride interfaces the hook implements
    private static List<Method> interfaceMethods( Class c ) {
        List<Method> methods = new ArrayList<>();
        for (; c != null; c = c.getSuperclass()) {
            for (Class i : c.getInterfaces()) {
                Class enclosing = i.getEnclosingClass();
                if (enclosing == null || !enclosing.getSimpleName().startsWith("BOverride"))
                    continue;
                for (Method m : i.getMethods()) {
                    if (!m.isDefault() && !Modifier.isStatic(m.getModifiers()))
                        methods.add(m);
                }
            }
        }
        return methods;
    }

    /// Public functions of the type a hook creates, without Object's
    private static List<Method> publicMethods( Class type ) {
        List<Method> methods = new ArrayList<>();
        for (Method m : type.getMethods()) {
            if (m.getDeclaringClass() != Object.class && !Modifier.isStatic(m.getModifiers()))
                methods.add(m);
        }
        return methods;
    }
}
