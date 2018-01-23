/**
 * @file
 */

#include "AbstractDatabaseTest.h"
#include "TestModel.h"
#include "persistence/ConnectionPool.h"
#include "persistence/DBHandler.h"
#include "engine-config.h"

namespace persistence {

class DatabaseModelTest: public AbstractDatabaseTest {
private:
	using Super = AbstractDatabaseTest;
protected:
	bool _supported = true;
	persistence::DBHandler _dbHandler;
public:
	void SetUp() override {
		Super::SetUp();
		_supported = _dbHandler.init();
		if (_supported) {
			ASSERT_TRUE(_dbHandler.createTable(db::TestModel())) << "Could not create table";
			ASSERT_TRUE(_dbHandler.dropTable(db::TestModel()));
			ASSERT_TRUE(_dbHandler.createTable(db::TestModel())) << "Could not create table";
			Log::debug("DatabaseModelTest: Finished setup");
		} else {
			Log::warn("DatabaseModelTest is skipped");
		}
	}

	void TearDown() override {
		Super::TearDown();
		_dbHandler.shutdown();
	}

	db::TestModel m(const std::string& email, const std::string& password) const {
		db::TestModel mdl;
		mdl.setName(email);
		mdl.setEmail(email);
		mdl.setPassword(password);
		mdl.setRegistrationdate(persistence::Timestamp::now());
		return mdl;
	}

	void massInsert(int amount) {
		if (!_supported) {
			return;
		}
		std::vector<db::TestModel> models(amount);
		std::vector<const Model*> modelPtrs(amount);
		for (int i = 0; i < amount; ++i) {
			models[i] = m(core::string::format("mail%i", i), "secret");
			modelPtrs[i] = &models[i];
		}
		EXPECT_TRUE(_dbHandler.insert(modelPtrs));
		int count = 0;
		_dbHandler.select(db::TestModel(), persistence::DBConditionOne(), [&] (db::TestModel&& model) {
			++count;
		});
		EXPECT_EQ(count, amount);
	}

	void createModel(const std::string& email, const std::string& password, int64_t& id) {
		ASSERT_TRUE(_supported);
		db::TestModel mdl = m(email, password);
		EXPECT_EQ(0, mdl.id());
		ASSERT_TRUE(_dbHandler.insert(mdl));
		EXPECT_NE(0, mdl.id());

		db::TestModel mdl2nd;
		const db::DBConditionTestModelEmail emailCond(email);
		const db::DBConditionTestModelPassword passwordCond(password);
		ASSERT_TRUE(_dbHandler.select(mdl2nd, persistence::DBConditionMultiple(true, {&emailCond, &passwordCond})));
		EXPECT_GT(mdl2nd.registrationdate().seconds(), uint64_t(0));
		EXPECT_EQ(mdl2nd.email(), mdl.email());
		ASSERT_EQ(mdl2nd.id(), mdl.id());

		db::TestModel mdl3nd;
		ASSERT_TRUE(_dbHandler.select(mdl3nd, db::DBConditionTestModelId(mdl.id())));
		EXPECT_GT(mdl3nd.registrationdate().seconds(), uint64_t(0));
		EXPECT_EQ(mdl3nd.email(), mdl.email());
		ASSERT_EQ(mdl3nd.id(), mdl.id());

		id = mdl.id();
	}
};

TEST_F(DatabaseModelTest, testCreateModel) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	createModel("testCreateModel@b.c.d", "secret", id);
}

TEST_F(DatabaseModelTest, testCreateModels) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	for (int i = 0; i < 5; ++i) {
		createModel(core::string::format("testCreateModels%i@b.c.d", i), "secret", id);
	}
}

TEST_F(DatabaseModelTest, testSelectAll) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	const int expected = 5;
	for (int i = 0; i < expected; ++i) {
		createModel(core::string::format("testSelectAll%i@b.c.d", i), "secret", id);
	}
	int count = 0;
	EXPECT_TRUE(_dbHandler.select(db::TestModel(), persistence::DBConditionOne(), [&] (db::TestModel&& model) {
		++count;
	}));
	EXPECT_EQ(count, expected);
}

TEST_F(DatabaseModelTest, testSelectByEmail) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	const int expected = 5;
	for (int i = 0; i < expected; ++i) {
		createModel(core::string::format("testSelectByEmail%i@b.c.d", i), "secret", id);
	}
	int count = 0;
	const db::DBConditionTestModelEmail condition("testSelectByEmail1@b.c.d");
	EXPECT_TRUE(_dbHandler.select(db::TestModel(), condition, [&] (db::TestModel&& model) {
		++count;
		ASSERT_EQ(std::string(condition.value(0)), model.email());
	}));
	EXPECT_EQ(count, 1);
}

TEST_F(DatabaseModelTest, testSelectById) {
	if (!_supported) {
		return;
	}
	const int expected = 5;
	int64_t id = -1L;
	for (int i = 0; i < expected; ++i) {
		createModel(core::string::format("testSelectById%i@b.c.d", i), "secret", id);
	}
	int count = 0;
	const db::DBConditionTestModelId condition(id);
	EXPECT_TRUE(_dbHandler.select(db::TestModel(), condition, [&] (db::TestModel&& model) {
		++count;
		ASSERT_EQ(id, model.id());
	}));
	EXPECT_EQ(count, 1);
}

TEST_F(DatabaseModelTest, testTruncate) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	createModel("testTruncate@b.c.d", "secret", id);
	EXPECT_TRUE(_dbHandler.truncate(db::TestModel()));
	int count = 0;
	_dbHandler.select(db::TestModel(), persistence::DBConditionOne(), [&] (db::TestModel&& model) {
		++count;
	});
	EXPECT_EQ(count, 0);
}

TEST_F(DatabaseModelTest, testDeleteConditionAndPrimaryKey) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	createModel("testDelete@b.c.d", "secret", id);
	db::TestModel model;
	model.setId(id);
	EXPECT_TRUE(_dbHandler.deleteModel(model, db::DBConditionTestModelEmail("testDelete@b.c.d")));
	int count = 0;
	_dbHandler.select(db::TestModel(), persistence::DBConditionOne(), [&] (db::TestModel&& model) {
		++count;
	});
	EXPECT_EQ(count, 0);
}

TEST_F(DatabaseModelTest, testDelete) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	createModel("testDelete@b.c.d", "secret", id);
	EXPECT_TRUE(_dbHandler.deleteModel(db::TestModel(), db::DBConditionTestModelId(id)));
	int count = 0;
	_dbHandler.select(db::TestModel(), persistence::DBConditionOne(), [&] (db::TestModel&& model) {
		++count;
	});
	EXPECT_EQ(count, 0);
}

TEST_F(DatabaseModelTest, testMultipleInsert) {
	if (!_supported) {
		return;
	}
	db::TestModel m1 = m("mail1", "password1");
	db::TestModel m2 = m("mail2", "password2");
	db::TestModel m3 = m("mail3", "password3");
	std::vector<const Model*> models{&m1, &m2, &m3};
	EXPECT_TRUE(_dbHandler.insert(models));
	int count = 0;
	_dbHandler.select(db::TestModel(), persistence::DBConditionOne(), [&] (db::TestModel&& model) {
		++count;
	});
	EXPECT_EQ(count, 3);
	count = 0;
	std::string email3;
	EXPECT_TRUE(_dbHandler.select(db::TestModel(), db::DBConditionTestModelEmail("mail3"), [&] (db::TestModel&& model) {
		++count;
	}));
	EXPECT_EQ(count, 1);
}

TEST_F(DatabaseModelTest, test1000Insert) {
	massInsert(1000);
}

TEST_F(DatabaseModelTest, test100Insert) {
	massInsert(100);
}

TEST_F(DatabaseModelTest, test10Insert) {
	massInsert(10);
}

TEST_F(DatabaseModelTest, testUpdate) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	createModel("testupdate@b.c.d", "secret", id);
	int count = 0;
	db::TestModel copy;
	_dbHandler.select(db::TestModel(), db::DBConditionTestModelId(id), [&] (db::TestModel&& model) {
		++count;
		copy = std::move(model);
	});
	ASSERT_EQ(count, 1);
	ASSERT_EQ("testupdate@b.c.d", copy.email());
	copy.setEmail("no@mail.com");
	_dbHandler.update(copy);
	_dbHandler.select(db::TestModel(), db::DBConditionTestModelId(id), [&] (db::TestModel&& model) {
		++count;
		ASSERT_EQ(copy.email(), model.email());
	});
}

TEST_F(DatabaseModelTest, testTimestamp) {
	if (!_supported) {
		return;
	}
	db::TestModel u;
	EXPECT_EQ(0, u.id());
	u.setEmail("testTimestamp@now.de");
	u.setName("testtimestampname");
	u.setPassword("testtimestamppassword");
	const auto now = _testApp->timeProvider()->tickMillis();
	u.setRegistrationdate(now / 1000UL);
	ASSERT_TRUE(_dbHandler.insert(u));

	_dbHandler.select(db::TestModel(), db::DBConditionTestModelId(u.id()), [=] (db::TestModel&& model) {
		const persistence::Timestamp& ts = model.registrationdate();
		const persistence::Timestamp tsNow(now / 1000);
		ASSERT_NEAR(ts.millis(), now, 10) << "db: " << ts.toString() << " now: " << tsNow.toString();
	});
}

TEST_F(DatabaseModelTest, testLimitOrderBy) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	for (int i = 0; i < 5; ++i) {
		createModel(core::string::format("testLimitOrderBy%i@b.c.d", i), "secret", id);
	}
	const int limit = 2;
	int count = 0;
	const persistence::OrderBy orderBy(db::TestModel::f_id(), persistence::Order::DESC, limit);
	_dbHandler.select(db::TestModel(), orderBy, [&] (db::TestModel&& model) {
		++count;
	});
	ASSERT_EQ(limit, count);
}

TEST_F(DatabaseModelTest, testOffsetOrderBy) {
	if (!_supported) {
		return;
	}
	int64_t id = -1L;
	const int n = 5;
	for (int i = 0; i < n; ++i) {
		createModel(core::string::format("testOffsetOrderBy%i@b.c.d", i), "secret", id);
	}
	const int limit = -1;
	const int offset = 3;
	int count = 0;
	const persistence::OrderBy orderBy(db::TestModel::f_id(), persistence::Order::DESC, limit, offset);
	_dbHandler.select(db::TestModel(), orderBy, [&] (db::TestModel&& model) {
		++count;
	});
	ASSERT_EQ(n - offset, count);
}

}
