import QtQuick 2.3
import QtTest 1.0
import QtLevelDB 1.0

//TODO: tests with multiple instances of LevelDB
    TestCase {
        id:testcase
        property url source: "test.db"
        name: "LevelDB"
        when: db.opened

        LevelDB{
            id:db
            source: testcase.source
            options{
                compressionType: Options.SnappyCompression
                createIfMissing: true
                errorIfExists: false
                paranoidChecks: false
            }
        }

        function init() {
            db.source = source
        }

        function cleanupTestCase() {
            db.destroyDB(source)
        }

        function test_create() {
            compare(db.opened, true, db.statusText)
        }

        function test_destroy() {
            db.source = Qt.resolvedUrl("test2.db")
            compare(db.opened, true, db.statusText)
            compare(db.destroyDB(Qt.resolvedUrl("test2.db")), true)
        }

        // TODO: add this test, but remove "lost" folder at the end
        //    function test_repair() {
        //        compare(db.repairDB(source), LevelDB.Ok)
        //    }

        function test_get_default(){
            compare(db.opened, true, db.statusText)
            compare(db.get("invalidKey", [{"id": "a"}]), [{"id": "a"}])
        }

        function test_put_get(){
            compare(db.opened, true, db.statusText)
            compare(db.putSync("asdf", ["fdfd"]), true, "should be ok")
            compare(db.get("asdf"), ["fdfd"])
        }

        function test_delete(){
            compare(db.opened, true, db.statusText)
            compare(db.put("asdf", ["asdf"]), true)
            compare(db.del("asdf"), true)
            compare(db.get("asdf"), undefined)
        }

        function test_batch(){
            compare(db.opened, true, db.statusText)
            var status = db.batch()
            .put("asdf","asdf")
            .put("string", "asdf2")
            .put("list", ["asdf"])
            .put("object", {"a": "b"})
            .del("asdf")
            .write()
            compare(status, true, "Batch Operation Failed")
            compare(db.get("asdf"), undefined)
            compare(db.get("string"),"asdf2", "Wrong data")
            compare(db.get("list"),["asdf"], "Wrong data")
            compare(db.get("object"),{"a": "b"}, "Wrong data")
        }
    }