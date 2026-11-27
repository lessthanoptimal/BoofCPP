import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Assertions.assertFalse
import org.junit.jupiter.api.Assertions.assertTrue
import org.junit.jupiter.api.Test
import org.junit.jupiter.api.io.TempDir
import java.io.File

class BenchmarkReportTest {
    @TempDir
    lateinit var dir: File

    /// A JMH result of ThresholdingBenchmark
    private fun result( op: String, impl: String, threads: Int, score: Double, type: String = "U8" ) =
        result("ThresholdingBenchmark.$op", mapOf("impl" to impl, "threads" to "$threads", "type" to type), score)

    private fun result( benchmark: String, params: Map<String, String>, score: Double ) = """
        {"benchmark": "org.boofcpp.benchmark.$benchmark",
         "params": {${params.entries.joinToString(", ") { "\"${it.key}\": \"${it.value}\"" }}},
         "primaryMetric": {"score": $score, "scoreError": 0.25, "scoreUnit": "ms/op"}}"""

    private fun write( impl: String, vararg results: String ) {
        File(dir, "$impl.json").writeText(results.joinToString(",", "[", "]"))
    }

    private fun metadata( impl: String, cpu: String ) {
        File(dir, "$impl.metadata.json").writeText("""{"date": "2026-11-27", "cpu": "$cpu", "jdk": "25",
            "boofcv": {"version": "1.6", "sha": "0123456789", "dirty": false},
            "boofcpp": {"version": "0.3", "sha": "abcdef0123", "dirty": true}, "native": "gcc"}""")
    }

    /// Rows pair each op and parameters across implementations, sorted with numbers compared as numbers, and a
    /// speedup below MIN_SPEEDUP is in bold
    @Test fun table() {
        write("java", result("otsu", "java", 16, 15.0), result("otsu", "java", 4, 3.0))
        write("strict", result("otsu", "strict", 16, 5.0), result("otsu", "strict", 4, 2.9))
        metadata("java", "cpu")
        metadata("strict", "cpu")
        val lines = BenchmarkReport.render(dir).lines()

        val header = lines.indexOf("| Op | threads | type | java (ms/op) | strict (ms/op) | strict speedup |")
        assertTrue(header >= 0, lines.joinToString("\n"))
        assertEquals("| otsu | 4 | U8 | 3.00 ± 0.25 | 2.90 ± 0.25 | **1.03x** |", lines[header + 2])
        assertEquals("| otsu | 16 | U8 | 15.0 ± 0.3 | 5.00 ± 0.25 | 3.00x |", lines[header + 3])
        val build = "BoofCV 1.6 0123456, BoofCPP 0.3 abcdef0 (dirty), gcc"
        assertTrue(lines.contains("| java | 2026-11-27 | $build | cpu | 25 |"))
        assertFalse(lines.any { it.startsWith("**Warning") })
    }

    /// Different CPUs give a warning, and an implementation without metadata says so
    @Test fun metadataChecks() {
        write("java", result("otsu", "java", 1, 15.0))
        write("strict", result("otsu", "strict", 1, 5.0))
        write("fast", result("otsu", "fast", 1, 4.0))
        metadata("java", "a")
        metadata("strict", "b")
        val lines = BenchmarkReport.render(dir).lines()
        assertTrue(lines.any { it.startsWith("**Warning") })
        assertTrue(lines.contains("| fast | metadata missing | | | |"))
    }

    /// Types are columns in their fixed order, Java's row has its time and the others their speedup, and ops
    /// without an image type, and parameters that vary in a benchmark, are in their own table and in the op
    @Test fun summary() {
        val variants = { impl: String, variant: String, score: Double ->
            result("DisparityCensusVariantsBenchmark.block_match",
                mapOf("impl" to impl, "threads" to "1", "variant" to variant, "size" to "9"), score)
        }
        write("java", result("otsu", "java", 1, 15.0, "F32"), result("otsu", "java", 1, 12.0),
            variants("java", "A", 20.0), variants("java", "B", 30.0))
        write("strict", result("otsu", "strict", 1, 15.0, "F32"), result("otsu", "strict", 1, 4.0),
            variants("strict", "A", 10.0), variants("strict", "B", 10.0))
        val lines = BenchmarkReport.render(dir).lines()

        val typed = lines.indexOf("| impl | op | threads | U8 | F32 |")
        assertTrue(typed >= 0, lines.joinToString("\n"))
        assertEquals("| java | Thresholding.otsu | 1 | 12.0 ms | 15.0 ms |", lines[typed + 2])
        assertEquals("| strict | Thresholding.otsu | 1 | 3.00x | **1.00x** |", lines[typed + 3])
        val untyped = lines.indexOf("| impl | op | threads | time |")
        assertEquals("| java | DisparityCensusVariants.block_match variant=A | 1 | 20.0 ms |", lines[untyped + 2])
        assertEquals("| strict | DisparityCensusVariants.block_match variant=A | 1 | 2.00x |", lines[untyped + 3])
        assertEquals("| strict | DisparityCensusVariants.block_match variant=B | 1 | 3.00x |", lines[untyped + 5])
    }

    /// The page has the results as JSON in place of the template's placeholder, with "</" escaped
    @Test fun html() {
        write("java", result("otsu</script>", "java", 1, 15.0))
        metadata("java", "cpu")
        val html = BenchmarkReport.renderHtml(dir)
        assertFalse(html.contains("/*DATA*/"))
        assertFalse(html.contains("otsu</script>"))
        assertTrue(html.contains("""label":"Thresholding.otsu<\/script>"""), html)
        assertTrue(html.contains("""score":15.0"""))
    }

    @Test fun number() {
        assertEquals("0.584", BenchmarkReport.number(0.5841))
        assertEquals("5.84", BenchmarkReport.number(5.841))
        assertEquals("58.4", BenchmarkReport.number(58.41))
        assertEquals("584", BenchmarkReport.number(584.1))
    }
}
