#include "stdafx.h"
#include "CppUnitTest.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace search::lp;
using namespace search;

namespace UnitTests
{
    TEST_CLASS(test_lifelong_planning)
    {
    public:
        TEST_METHOD(cost_function)
        {
            Assert::AreEqual(1, cost());
        }

        TEST_METHOD(infinity_function)
        {
            Assert::AreEqual(10'000, infinity());
        }

        TEST_METHOD(key)
        {
            Key key{ 42, 99 };
            Assert::AreEqual(42, key.fst);
            Assert::AreEqual(99, key.snd);
            Assert::IsTrue(Key{ 1, 2 } < Key{ 2, 1 });
            Assert::IsTrue(Key{ 2, 1 } < Key{ 2, 2 });
            Assert::IsTrue(Key{ 2, 2 } == Key{ 2, 2 });
        }

        TEST_METHOD(lp_cell)
        {
            Cell c{ 42, 99 };
            Assert::AreEqual(42, c.row);
            Assert::AreEqual(99, c.col);
            Assert::IsTrue(Cell{ 1, 1 } == Cell{ 1, 1 });
            Assert::IsTrue(Cell{ 1, 2 } != Cell{ 1, 1 });
            Assert::AreEqual(string{ "[r=42,c=99]" }, c.to_string());
            Assert::AreEqual(69382224u, c.to_hash());
            Assert::AreEqual(69382224u, std::hash<Cell>{}(c));

            {//test neighbour
                Cell c{ 1, 1 };
                decltype(c.neighbours()) expect = 
                {
                    { 0, 0 }, { 0, 1 }, { 0, 2 },
                    { 1, 0 },   /* */   { 1, 2 },
                    { 2, 0 }, { 2, 1 }, { 2, 2 }
                };

                for (auto i = 1; i != expect.size(); ++i)
                    Assert::IsTrue(expect[i] == c.neighbours()[i]);
            }

            {//to confirm hash works
                unordered_set<Cell> blockeds;
                blockeds.insert({ 1, 2 });
                blockeds.insert({ 1, 2 });
                blockeds.insert({ 1, 3 });
                Assert::AreEqual(2u, blockeds.size());
            }
        }

        TEST_METHOD(lp_heuristics)
        {
            Assert::AreEqual(4, HEURISTICS.at("manhattan")({ 1, 3 }, { 5, 0 }));
            Assert::AreEqual(5, HEURISTICS.at("manhattan")({ 0, 2 }, { 5, 0 }));

            Assert::AreEqual(5, HEURISTICS.at("euclidean")({ 6, 5 }, { 9, 9 }));
            Assert::AreEqual(1, HEURISTICS.at("euclidean")({ 8, 8 }, { 9, 9 }));
        }

        TEST_METHOD(lp_state)
        {
            auto ls = LpState{ { 3, 4 }, 6, 7, 1, false };
            Assert::AreEqual(3, ls.cell.row);
            Assert::AreEqual(4, ls.cell.col);
            Assert::AreEqual(6, ls.g);
            Assert::AreEqual(7, ls.r);
            Assert::IsTrue(ls == LpState{ { 3, 4 }, 6, 7, 1 });
            Assert::IsTrue(ls == LpState{ { 3, 4 }, 6, 7, 1, false });
            Assert::IsFalse(ls.bad);
            Assert::AreEqual(string{ "{[r=3,c=4]|g:6|r:7|h:1|b:f}" }, ls.to_string());
        }

        TEST_METHOD(matrix_class)
        {
            Matrix matrix{ 9, 8 };
            Assert::AreEqual(9u, matrix.rows());
            Assert::AreEqual(8u, matrix.cols());
            Assert::AreEqual(1'0000, infinity());

            {
                Cell c = { 2, 4 };
                Assert::AreEqual(infinity(), matrix.at(c).g);
                Assert::AreEqual(infinity(), matrix.at(c).r);
                Assert::IsTrue(c == matrix.at(c).cell);
            }

            {
                Cell c = { 4, 2 };
                Assert::AreEqual(infinity(), matrix.at(c).g);
                Assert::AreEqual(infinity(), matrix.at(c).r);
                Assert::IsTrue(c == matrix.at(c).cell);
            }

            {//for testing to_string
                Matrix matrix{ 2, 2 };
                string expect = "{[r=0,c=0]|g:10000|r:10000|h:0|b:f}{[r=1,c=0]|g:10000|r:10000|h:0|b:f}+++{[r=0,c=1]|g:10000|r:10000|h:0|b:f}{[r=1,c=1]|g:10000|r:10000|h:0|b:f}+++";
                Assert::AreEqual(expect, matrix.to_string());
            }
        }

        TEST_METHOD(matrix_of_lpastar)
        {
            //case from pdf file
            unordered_set<Cell> bad_cells{ { 1, 0 },{ 2, 0 },{ 3, 0 },{ 4, 0 },{ 1, 2 },{ 2, 2 },{ 3, 2 },{ 4, 2 } };
            LpAstarCore lpa{ 6, 4, { 0, 3 }, { 5, 0 }, "manhattan", bad_cells };

            //test bad marking
            for (auto cell : bad_cells)
                Assert::AreEqual(true, lpa.matrix.at(cell).bad);
            for (auto r = 0; r != lpa.matrix.rows(); ++r)
                for (auto c = 0; c != lpa.matrix.cols(); ++c)
                    if(bad_cells.count({ r, c }) == 0)
                        Assert::AreEqual(false, lpa.matrix.at({ r, c }).bad);

            //test h value marking
            Assert::AreEqual(5, lpa.matrix.at({ 0, 2 }).h);
            Assert::AreEqual(4, lpa.matrix.at({ 1, 3 }).h);
        }

        TEST_METHOD(priority_queue_of_lpastar)
        {
            {//pushing order 1
                unordered_set<Cell> bad_cells{ { 1, 0 },{ 2, 0 },{ 3, 0 },{ 4, 0 },{ 1, 2 },{ 2, 2 },{ 3, 2 },{ 4, 2 } };
                LpAstarCore lpa{ 6, 4, { 0, 3 }, { 5, 0 }, "manhattan", bad_cells };
                lpa.q.push({ 0, 2 });
                lpa.q.push({ 1, 3 });
                Assert::IsTrue(Cell{ 1, 3 } == lpa.q.top());
            }

            {//pushing order 2
                unordered_set<Cell> bad_cells{ { 1, 0 },{ 2, 0 },{ 3, 0 },{ 4, 0 },{ 1, 2 },{ 2, 2 },{ 3, 2 },{ 4, 2 } };
                LpAstarCore lpa{ 6, 4,{ 0, 3 },{ 5, 0 }, "manhattan", bad_cells };
                lpa.q.push({ 1, 3 });
                lpa.q.push({ 0, 2 });
                Assert::IsTrue(Cell{ 1, 3 } == lpa.q.top());
            }
        }
    };
}