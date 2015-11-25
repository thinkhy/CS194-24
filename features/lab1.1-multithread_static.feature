@multi
Feature: Multithreaded HTTP Server
  
  Background:
    Given the root path is "http://localhost:8088"

  @html @single
  Scenario: Get the test page
		When I visit "/test.html"
		Then I should see "A test page"

        @html @single
	Scenario: Get the Lorem page
		When I visit "/lorem.html"
		Then I should see "Quisque sit amet congue elit"

	Scenario: Get same page multiple times
		When I visit "/test.html"
		Then I should see "A test page"
		When I visit "/test.html"
		Then I should see "A test page"
		When I visit "/test.html"
		Then I should see "A test page"

	Scenario Outline: Get different pages multiple times
		When I visit "<url>"
		Then I should see "<content>"

	Examples:
		| url         | content                      | 
		| /test.html  | A test page                  | 
		| /lorem.html | Quisque sit amet congue elit | 
		| /webclock   | 00 +0000                     |

        @cgi @single
	Scenario: Check CGI
		When I visit "/webclock"
		Then I should see "00 +0000"

        @cgi @multi
	Scenario: Check multiple CGI requests
		When I visit "/webclock"
		Then I should see "00 +0000"
		When I visit "/webclock"
		Then I should see "00 +0000"
		When I visit "/webclock"
		Then I should see "00 +0000"
		When I visit "/webclock"
		Then I should see "00 +0000"
		When I visit "/webclock"
		Then I should see "00 +0000"

	Scenario: Check interleaved CGI and static reqeusts
		When I visit "/webclock"
		Then I should see "00 +0000"
		When I visit "/lorem.html"
		Then I should see "Quisque sit amet congue elit"
		When I visit "/webclock"
		Then I should see "00 +0000"
		When I visit "/lorem.html"
		Then I should see "Quisque sit amet congue elit"
		When I visit "/webclock"
		Then I should see "00 +0000"
		When I visit "/lorem.html"
		Then I should see "Quisque sit amet congue elit"		
