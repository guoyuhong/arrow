# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

class CSVLoaderTest < Test::Unit::TestCase
  include Helper::Fixture

  def load_csv(input)
    Arrow::CSVLoader.load(input, skip_lines: /^#/)
  end

  sub_test_case(".load") do
    test("String: data: with header") do
      data = fixture_path("with-header-float.csv").read
      assert_equal(<<-TABLE, load_csv(data).to_s)
	name	     score
0	alice	 10.100000
1	bob 	 29.200000
2	chris	 -1.300000
      TABLE
    end

    test("String: data: without header") do
      data = fixture_path("without-header-float.csv").read
      assert_equal(<<-TABLE, load_csv(data).to_s)
	0	         1
0	alice	 10.100000
1	bob	 29.200000
2	chris	 -1.300000
      TABLE
    end

    test("String: path: with header") do
      path = fixture_path("with-header-float.csv").to_s
      assert_equal(<<-TABLE, load_csv(path).to_s)
	name	     score
0	alice	 10.100000
1	bob 	 29.200000
2	chris	 -1.300000
      TABLE
    end

    test("String: path: without header") do
      path = fixture_path("without-header-float.csv").to_s
      assert_equal(<<-TABLE, load_csv(path).to_s)
	0	         1
0	alice	 10.100000
1	bob	 29.200000
2	chris	 -1.300000
      TABLE
    end

    test("Pathname: with header") do
      path = fixture_path("with-header-float.csv")
      assert_equal(<<-TABLE, load_csv(path).to_s)
	name	     score
0	alice	 10.100000
1	bob 	 29.200000
2	chris	 -1.300000
      TABLE
    end

    test("Pathname: without header") do
      path = fixture_path("without-header-float.csv")
      assert_equal(<<-TABLE, load_csv(path).to_s)
	0	         1
0	alice	 10.100000
1	bob	 29.200000
2	chris	 -1.300000
      TABLE
    end

    test("null: with double quote") do
      path = fixture_path("null-with-double-quote.csv").to_s
      assert_equal(<<-TABLE, load_csv(path).to_s)
	name	score
0	alice	   10
1	bob 	     
2	chris	   -1
      TABLE
    end

    test("null: without double quote") do
      path = fixture_path("null-without-double-quote.csv").to_s
      assert_equal(<<-TABLE, load_csv(path).to_s)
	name	score
0	alice	   10
1	bob 	     
2	chris	   -1
      TABLE
    end

    test("number: float, integer") do
      path = fixture_path("float-integer.csv").to_s
      assert_equal([2.9, 10, -1.1],
                   load_csv(path)[:score].to_a)
    end

    test("number: integer, float") do
      path = fixture_path("integer-float.csv").to_s
      assert_equal([10.0, 2.9, -1.1],
                   load_csv(path)[:score].to_a)
    end
  end
end
