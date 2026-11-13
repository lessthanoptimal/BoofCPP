package org.boofcpp;

import boofcv.factory.disparity.ConfigDisparityBM;
import boofcv.factory.disparity.ConfigDisparityError;
import boofcv.factory.disparity.DisparityError;
import boofcv.factory.transform.census.CensusVariants;
import boofcv.struct.ConfigLength;
import boofcv.struct.Configuration;
import boofcv.struct.border.BorderType;
import org.junit.jupiter.api.Test;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.util.*;
import java.util.function.BiConsumer;
import java.util.function.Consumer;
import java.util.function.Supplier;

import static org.junit.jupiter.api.Assertions.*;

/// Makes sure the C++ configurations have the same fields, enum values, and defaults as BoofCV's
public class TestNativeConfigs {
    @Test void configDisparityBM() throws Exception {
        checkConfig(ConfigDisparityBM.class, ConfigDisparityBM::new, NativeConfigs.fieldNamesBM(), Set.of(),
                NativeConfigs::defaultsBM, NativeConfigs::roundTripBM);
    }

    @Test void configCensus() throws Exception {
        checkConfig(ConfigDisparityError.Census.class, ConfigDisparityError.Census::new,
                NativeConfigs.fieldNamesCensus(), Set.of(), NativeConfigs::defaultsCensus,
                NativeConfigs::roundTripCensus);
    }

    @Test void configNCC() throws Exception {
        checkConfig(ConfigDisparityError.NCC.class, ConfigDisparityError.NCC::new,
                NativeConfigs.fieldNamesNCC(), Set.of(), NativeConfigs::defaultsNCC, NativeConfigs::roundTripNCC);
    }

    @Test void configLength() throws Exception {
        checkConfig(ConfigLength.class, ConfigLength::new, NativeConfigs.fieldNamesLength(), Set.of(),
                NativeConfigs::defaultsLength, NativeConfigs::roundTripLength);
    }

    @Test void enumNames() {
        assertArrayEquals(names(DisparityError.values()), NativeConfigs.disparityErrorNames());
        assertArrayEquals(names(BorderType.values()), NativeConfigs.borderTypeNames());
        assertArrayEquals(names(CensusVariants.values()), NativeConfigs.censusVariantsNames());
    }

    /// Same field names, same defaults, and every field survives a round trip through C++
    ///
    /// @param notSupported Java fields that C++ doesn't have yet
    <C> void checkConfig( Class<C> type, Supplier<C> factory, String[] cppNames, Set<String> notSupported,
                          Consumer<C> cppDefaults, BiConsumer<C, C> roundTrip ) throws Exception {
        // field names
        Set<String> java = publicFields(type);
        Set<String> cpp = new HashSet<>(Arrays.asList(cppNames));
        assertEquals(cppNames.length, cpp.size(), "C++ has duplicate names");

        Set<String> missing = new TreeSet<>(java);
        missing.removeAll(cpp);
        missing.removeAll(notSupported);
        assertTrue(missing.isEmpty(), "Java fields missing in C++: " + missing);

        Set<String> extra = new TreeSet<>(cpp);
        extra.removeAll(java);
        assertTrue(extra.isEmpty(), "C++ fields not in Java: " + extra);

        // Make sure the allow list doesn't hide fields that C++ now has
        for (String name : notSupported) {
            assertTrue(java.contains(name), name);
            assertFalse(cpp.contains(name), name);
        }

        // defaults
        C expected = factory.get();
        C found = factory.get();
        setNotDefault(found, cppNames);
        cppDefaults.accept(found);
        for (String name : cppNames) {
            Field f = type.getField(name);
            assertTrue(sameValues(f.get(expected), f.get(found)), name);
        }

        // round trip
        C src = factory.get();
        setNotDefault(src, cppNames);
        C dst = factory.get();
        roundTrip.accept(src, dst);
        for (String name : cppNames) {
            Field f = type.getField(name);
            // sanity check, otherwise a field that isn't copied would pass
            assertFalse(sameValues(f.get(expected), f.get(src)), name);
            assertTrue(sameValues(f.get(src), f.get(dst)), name);
        }
    }

    static Set<String> publicFields( Class<?> type ) {
        Set<String> ret = new HashSet<>();
        for (Field f : type.getFields()) {
            if (!Modifier.isStatic(f.getModifiers()))
                ret.add(f.getName());
        }
        return ret;
    }

    static String[] names( Enum<?>[] values ) {
        return Arrays.stream(values).map(Enum::name).toArray(String[]::new);
    }

    /// Compares values. Nested configurations don't implement equals(), so their public fields are compared
    static boolean sameValues( Object a, Object b ) throws Exception {
        if (!(a instanceof Configuration))
            return Objects.equals(a, b);
        for (String name : publicFields(a.getClass())) {
            Field f = a.getClass().getField(name);
            if (!sameValues(f.get(a), f.get(b)))
                return false;
        }
        return true;
    }

    /// Changes every listed field to a value that isn't the default
    static void setNotDefault( Object config, String[] fields ) throws Exception {
        for (String name : fields) {
            Field f = config.getClass().getField(name);
            Class<?> type = f.getType();
            if (type == int.class) {
                f.setInt(config, f.getInt(config) + 7);
            } else if (type == double.class) {
                f.setDouble(config, f.getDouble(config) + 0.5);
            } else if (type == boolean.class) {
                f.setBoolean(config, !f.getBoolean(config));
            } else if (type.isEnum()) {
                Object[] values = type.getEnumConstants();
                f.set(config, values[(((Enum<?>)f.get(config)).ordinal() + 1)%values.length]);
            } else if (Configuration.class.isAssignableFrom(type)) {
                Object nested = f.get(config);
                setNotDefault(nested, publicFields(nested.getClass()).toArray(new String[0]));
            } else {
                fail("Unsupported field type " + type + " " + name);
            }
        }
    }
}
