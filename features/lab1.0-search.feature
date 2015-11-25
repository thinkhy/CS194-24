Feature: Basic browsing functions with Capybara 

  Capybara is configured to be integrated with
  Cucumber and uses mechanize to perform remote
  accesses that will allow us testing arbitrary
  webb applications. 
 
  Background:
    Given the root path is "http://www.bing.com"

  Scenario: Check connection
    When I am on the root page
    Then I should see "Bing"

  Scenario: Perform a simple search
    Given I am on the root page
    And I search for "TiD"
    Then I should see "www.tid.org.cn"

  Scenario: Ensure a string is not present
    Given I am on the root page
    And I search for "cats"
    Then I should not see "test automation"
