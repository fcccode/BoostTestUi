﻿//  (C) Copyright Gert-Jan de Vos 2012.
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at 
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://boosttestui.wordpress.com/ for the boosttestui home page.

namespace BoostTestUI
{
	using NUnit.Framework;

	[TestFixture, Category("NUnit"), Category("Sample")]
	public class NUnitSample
	{
		[Test]
		public void Success()
		{
			Assert.AreEqual(1 + 1, 2);
		}

		[Test]
		[Explicit]
		public void Fail()
		{
			Assert.AreEqual(1 + 1, 3);
		}

		[Test]
		[Explicit]
		public void Exception()
		{
			throw new System.Exception("Failed Test");
		}

		[Test]
		[ExpectedException("System.Exception")]
		public void ExpectedException()
		{
			throw new System.Exception("Failed Test");
		}

		[Test]
		[Explicit]
		[ExpectedException("System.InvalidCastException")]
		public void ExpectedException2()
		{
			throw new System.Exception("Failed Test");
		}

		[Test]
		[Explicit]
		[ExpectedException("System.Exception")]
		public void NoExpectedException()
		{
		}

		[Test]
		[Explicit]
		public void NullException()
		{
			string name = null;
			name.IndexOf('x');
		}

		[Test, Category("Waiting")]
		public void Wait1()
		{
			System.Console.WriteLine("Waiting");
			System.Threading.Thread.Sleep(1000);
		}

		[Test, Category("Waiting")]
		[Repeat(3)]
		public void Wait2()
		{
			System.Console.WriteLine("Waiting");
			System.Threading.Thread.Sleep(1000);
		}

		[Test, Category("Waiting")]
		public void Wait3()
		{
			System.Console.WriteLine("Waiting");
			System.Threading.Thread.Sleep(1000);
		}

		[Test, Category("Waiting")]
		public void Wait4()
		{
			System.Console.WriteLine("Waiting");
			System.Threading.Thread.Sleep(1000);
		}

		[Test, Category("Waiting")]
		public void Wait5<T>([Values(1, "aap", false)] T t)
		{
			System.Console.WriteLine("Waiting: " + t.ToString());
			System.Threading.Thread.Sleep(1000);
		}

		[Test]
		public void Values([Values("one", "two", "three")] string s)
		{
			System.Console.WriteLine("Value: \"{0}\"", s);
		}

		[Test]
		public void MultiplyCombinatorial([Values(1, 2, 3)] double a, [Values(4, 5, 6)] double b)
		{
			Assert.AreEqual(a * b, b * a);
			System.Console.WriteLine("{0} x {1} = {2}", a, b, a * b);
		}

		[Test, Sequential]
		public void MultiplySequential([Values(1, 2, 3)] double a, [Values(4, 5, 6)] double b, [Values("Ann", "Bob")] string name)
		{
			Assert.AreEqual(a * b, b * a);
			System.Console.WriteLine("{0}: {1} x {2} = {3}", name, a, b, a * b);
		}

		[Test]
		[Ignore("ignored test")]
		public void IgnoredTest()
		{
			throw new System.Exception();
		}
	}

	[TestFixture]
	public class SecondSample
	{
		[Test]
		public void SecondTest()
		{
			Assert.AreEqual(1 + 1, 2);
		}
	}

	[TestFixture]
	public class ErrorInConstructor
	{
		public ErrorInConstructor()
		{
			Assert.AreEqual(1 + 1, 3);
		}

		[Test]
		public void Test1()
		{
			Assert.AreEqual(1 + 1, 2);
		}

		[Test]
		public void Test2()
		{
			Assert.AreEqual(1 + 1, 2);
		}
	}

	public class TestFixtureSetUpBase
	{
		[TestFixtureSetUp]
		public void TestFixtureSetUp2()
		{
			System.Console.WriteLine("TestFixtureSetUp.TestFixtureSetUp");
		}
	}

	[TestFixture]
	public class ErrorInTestFixtureSetUp : TestFixtureSetUpBase
	{
		[TestFixtureSetUp]
		public void TestFixtureSetUp()
		{
			System.Console.WriteLine("TestFixtureSetUp");
			Assert.AreEqual(1 + 1, 3);
		}

		[Test]
		public void Test1()
		{
			Assert.AreEqual(1 + 1, 2);
		}

		[Test]
		public void Test2()
		{
			Assert.AreEqual(1 + 1, 2);
		}
	}

	public class SetUpBase
	{
		[SetUp]
		public void SetUp2()
		{
			System.Console.WriteLine("SetUpBase.SetUp");
		}
	}

	[TestFixture]
	public class ErrorInSetUp : SetUpBase
	{
		[SetUp]
		public void SetUp()
		{
			System.Console.WriteLine("SetUp");
			Assert.AreEqual(1 + 1, 3);
		}

		[Test]
		public void Test1()
		{
			Assert.AreEqual(1 + 1, 2);
		}

		[Test]
		public void Test2()
		{
			Assert.AreEqual(1 + 1, 2);
		}
	}

	[TestFixture]
	public class ErrorInTearDown
	{
		[TearDown]
		public void TearDown()
		{
			Assert.AreEqual(1 + 1, 3);
		}

		[Test]
		public void Test1()
		{
			Assert.AreEqual(1 + 1, 2);
		}

		[Test]
		public void Test2()
		{
			Assert.AreEqual(1 + 1, 2);
		}
	}

	[TestFixture]
	public class ErrorInTestFixtureTearDown
	{
		[TestFixtureTearDown]
		public void TestFixtureTearDown()
		{
			Assert.AreEqual(1 + 1, 3);
		}

		[Test]
		public void Test1()
		{
			Assert.AreEqual(1 + 1, 2);
		}

		[Test]
		public void Test2()
		{
			Assert.AreEqual(1 + 1, 2);
		}
	}
}
