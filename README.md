Commerce License Billing
========================

Commerce License Billing provides advanced (prepaid, postpaid, prorated, plan-based, metered)
recurring billing for licenses.

Dependencies: Bundleswitcher, Commerce License, Advanced queue.

Getting started
---------------
1. Go to admin/config/licenses/billing-cycle-types and add a billing cycle type.
2. Create a product, select a license type, then below select your billing cycle type
and billing type (prepaid or postpaid).
3. Checkout the product. If you selected postpaid as the billing type, your product
will be free.
4. A billing cycle has now been opened (with the current start date, and the end date
depending on your billing cycle type settings), along with a matching recurring order.
5. When the billing cycle expires, the recurring order will be closed and charged for
using Commerce Card on File, and a new billing cycle & order will be opened.

Relationship to Commerce License
--------------------------------
Any license type can be used for recurring billing without changes.
A license is considered billable if its product has a billing cycle type
selected.

If the license type wants to have metered billing, it must implement the
`CommerceLicenseBillingUsageInterface` interface.

By default, licenses are revisionable, but changes to a license don't create
new revisions. Commerce License Billing changes that logic for billable licenses,
ensuring that a new revision is created for status or product_id changes
(this is essential for later pricing and prorating).

Prepaid billing
---------------
Prepaid products are paid up front.

That means that if a customer registers on April 1st, he will immediately pay the
monthly fee for April. On the first day of May, he will be charged for the
april usage (if any), and the monthly fee for May. If on May 15h he cancels
his subscription, on the first day of June he will only pay the usage for May.

The other half of the May monthly fee will not be refunded, since that is
not currently implemented (a common strategy being to award the customer
points to be used for discounting future purchases).

Postpaid billing
----------------
Postpaid products are paid at the end of the billing cycle.

That means that if the customer registers on April 1st, his order is free and he pays
nothing. On the first day of May, he will be charged for the April monthly fee,
and the april usage (if any). If on May 15th he cancels his subscription, on the
first day of June he will pay the prorated montly fee for May, and the usage
for May.

Prorated payments
-----------------
A prorated payment is a payment proportional to the duration of the usage.
So, if the billing cycle is two weeks, but the plan was used for one week,
only half of the plan's price will be set on the line item.

The usage records have `start` and `end` timestamps.
Plans are priced by examining license revisions, each of which has
a `revision_created` and `revision_ended` timestamp, used the same way.

If the end timestamp is 0, it is assumed that the record is still active / in progress,
so the end of the billing cycle is taken as the end instead, in order to give a cost estimation.
The duration (end - start) is compared to the billing cycle duration, and the record is priced proportionally.

Plan-based billing
------------------
Each license has one plan at a given point of time, which is the referenced product.

Licenses are revisionable, and Commerce License Billing modifies the default
behavior so that new revisions are always created for product_id (plan) and
status changes. Each revision has a `revision_created` and `revision_ended` timestamp.
The `revision_ended` timestamp is 0 for the current revision.
The timestamps are used to prorate the price of each plan that was active
during the billing cycle.

Only revisions with status `COMMERCE_LICENSE_ACTIVE` are priced, so if a license
was inactive for a week, that period won't be priced since that revision will
be ignored.

Metered (usage-based) billing
-----------------------------
If a license type implements the `CommerceLicenseBillingUsageInterface` interface
and declares its usage groups, the module will allow usage to be
registered and calculated for each usage group separately, and charge for it
at the end of the billing cycle.

Usage is reported asynchronously, and it is your job to call
`commerce_license_billing_usage_add()` and register usage (after an API call
received through Services, or after contacting the service yourself on cron, etc).

There are two types of usage groups: counter and gauge.

- The counter tracks usage over time, and is always charged for in total.
For example if the following bandwidth usage was reported:
`Jan 1st - Jan 15th; 1024` and `Jan 15th - Jan 31st; 128`, there
will be one line item, charging for 1052mb of usage.

- The gauge tracks discrete usage values over time, and each
value is charged for separately. For example, if the following env
usage is reported `Jan 1st - Jan 15th; 2` and `Jan 15th - Jan 31st; 4`,
there will be two prorated line items, charging for 2 and 4 environments.
The gauge type also allows for open-ended usage (`immediate => TRUE`), in which
case it is carried over into the next billing cycles.

A usage group can also define `free_quantity`, the quantity provided for free
with the license. Only usage exceeding this quantity will be charged for.
For counters this means that the free quantity is subtracted from the total quantity.
For gauges this means that the gauge values that equal free_quantity are ignored.

The `free_quantity` value can be hardcoded, or taken from the license's product
(making it plan-based, which is a common use case, with Plan A providing one quantity
for free, and Plan B providing another quantity for free).
The module makes sure to price the usage according to the plan that was active
at the time.

See:

- `CommerceLicenseBillingUsageInterface`
- `commerce_license_billing_usage_add()`
- `commerce_license_billing_usage_clear()`
- `commerce_license_billing_current_usage()`
- `commerce_license_billing_usage_history_list()`

Recurring order refresh
-----------------------
The recurring order is refreshed each time it is loaded (`hook_commerce_order_load()`),
updating the line items (quantities, prices) based on the latest plan history and usage.

The start and end timestamps on the line items are maintained.

Billing cycle types
-------------------
Billing cycle types are exportable entities managed on admin/config/licenses/billing-cycle-types.
Each billing cycle type has a bundle, the billing cycle engine, powered by
[entity bundle plugin](https://drupal.org/project/entity_bundle_plugin).
This allows each billing cycle type to have methods for creating and naming new billing cycles.
See `CommerceLicenseBillingCycleEngineInterface` and `CommerceLicenseBillingCycleTypeBase`
for more information.

The module provides a "periodic" billing cycle engine that generates periodic
billing cycles (daily/weekly/monthly, synchronous or asynchronous).

Fields and bundles
------------------
The module creates the following fields on any product type that's license enabled:

- `cl_billing_cycle_type` - Reference to the billing cycle type.
- `cl_billing_type` - The billing type (prepaid / postpaid).

It provides the recurring line item type with the following additional fields:

- `cl_billing_start` - a datetime field, used for prorating.
- `cl_billing_end` - a datetime field, used for prorating.
- `cl_billing_license` - Reference to the license for which the line item was generated.

It provides the recurring order type with the following additional fields:

- `cl_billing_cycle` - Reference to the billing cycle.
