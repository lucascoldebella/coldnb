"use client";
import React from "react";
import Image from "next/image";
import ReviewSorting from "./ReviewSorting";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function Reviews() {
  const { t } = useLanguage();
  return (
    <>
      <div className="tab-reviews-heading">
        {" "}
        <div className="top">
          <div className="text-center">
            <div className="number title-display">4.9</div>
            <div className="list-star">
              <i className="icon icon-star" />
              <i className="icon icon-star" />
              <i className="icon icon-star" />
              <i className="icon icon-star" />
              <i className="icon icon-star" />
            </div>
            <p>{"(168 " + t("reviews.ratings") + ")"}</p>
          </div>
          <div className="rating-score">
            <div className="item">
              <div className="number-1 text-caption-1">5</div>
              <i className="icon icon-star" />
              <div className="line-bg">
                <div style={{ width: "94.67%" }} />
              </div>
              <div className="number-2 text-caption-1">59</div>
            </div>
            <div className="item">
              <div className="number-1 text-caption-1">4</div>
              <i className="icon icon-star" />
              <div className="line-bg">
                <div style={{ width: "60%" }} />
              </div>
              <div className="number-2 text-caption-1">46</div>
            </div>
            <div className="item">
              <div className="number-1 text-caption-1">3</div>
              <i className="icon icon-star" />
              <div className="line-bg">
                <div style={{ width: "0%" }} />
              </div>
              <div className="number-2 text-caption-1">0</div>
            </div>
            <div className="item">
              <div className="number-1 text-caption-1">2</div>
              <i className="icon icon-star" />
              <div className="line-bg">
                <div style={{ width: "0%" }} />
              </div>
              <div className="number-2 text-caption-1">0</div>
            </div>
            <div className="item">
              <div className="number-1 text-caption-1">1</div>
              <i className="icon icon-star" />
              <div className="line-bg">
                <div style={{ width: "0%" }} />
              </div>
              <div className="number-2 text-caption-1">0</div>
            </div>
          </div>
        </div>
        <div>
          <div className="btn-style-4 text-btn-uppercase letter-1 btn-comment-review btn-cancel-review">
            {t("reviews.cancelReview")}
          </div>
          <div className="btn-style-4 text-btn-uppercase letter-1 btn-comment-review btn-write-review">
            {t("reviews.writeReview")}
          </div>
        </div>
      </div>
      <div className="reply-comment style-1 cancel-review-wrap">
        <div className="d-flex mb_24 gap-20 align-items-center justify-content-between flex-wrap">
          <h4 className="">{"03 " + t("blog.comments")}</h4>
          <div className="d-flex align-items-center gap-12">
            <div className="text-caption-1">{t("reviews.sortBy")}</div>
            <ReviewSorting />
          </div>
        </div>
        <div className="reply-comment-wrap">
          <div className="reply-comment-item">
            <div className="user">
              <div className="image">
                <Image
                  alt=""
                  src="/images/avatar/user-default.jpg"
                  width={120}
                  height={120}
                />
              </div>
              <div>
                <h6>
                  <a href="#" className="link">
                    Superb quality apparel that exceeds expectations
                  </a>
                </h6>
                <div className="day text-secondary-2 text-caption-1">
                  1 days ago &nbsp;&nbsp;&nbsp;-
                </div>
              </div>
            </div>
            <p className="text-secondary">
              Great theme - we were looking for a theme with lots of built in
              features and flexibility and this was perfect. We expected to need
              to employ a developer to add a few finishing touches. But we
              actually managed to do everything ourselves. We did have one small
              query and the support given was swift and helpful.
            </p>
          </div>
          <div className="reply-comment-item type-reply">
            <div className="user">
              <div className="image">
                <Image
                  alt=""
                  src="/images/avatar/user-ColdnbMain.jpg"
                  width={104}
                  height={104}
                />
              </div>
              <div>
                <h6>
                  <a href="#" className="link">
                    {t("reviews.replyFromColdnb")}
                  </a>
                </h6>
                <div className="day text-secondary-2 text-caption-1">
                  1 days ago &nbsp;&nbsp;&nbsp;-
                </div>
              </div>
            </div>
            <p className="text-secondary">
              We love to hear it! Part of what we love most about ColdnbMain is how
              much it empowers store owners like yourself to build a beautiful
              website without having to hire a developer :) Thank you for this
              fantastic review!
            </p>
          </div>
          <div className="reply-comment-item">
            <div className="user">
              <div className="image">
                <Image
                  alt=""
                  src="/images/avatar/user-default.jpg"
                  width={120}
                  height={120}
                />
              </div>
              <div>
                <h6>
                  <a href="#" className="link">
                    Superb quality apparel that exceeds expectations
                  </a>
                </h6>
                <div className="day text-secondary-2 text-caption-1">
                  1 days ago &nbsp;&nbsp;&nbsp;-
                </div>
              </div>
            </div>
            <p className="text-secondary">
              Great theme - we were looking for a theme with lots of built in
              features and flexibility and this was perfect. We expected to need
              to employ a developer to add a few finishing touches. But we
              actually managed to do everything ourselves. We did have one small
              query and the support given was swift and helpful.
            </p>
          </div>
        </div>
      </div>
      <form
        className="form-write-review write-review-wrap"
        onSubmit={(e) => e.preventDefault()}
      >
        <div className="heading">
          <h4>{t("reviews.writeReview")}:</h4>
          <div className="list-rating-check">
            <input type="radio" id="star5" name="rate" defaultValue={5} />
            <label htmlFor="star5" title="text" />
            <input type="radio" id="star4" name="rate" defaultValue={4} />
            <label htmlFor="star4" title="text" />
            <input type="radio" id="star3" name="rate" defaultValue={3} />
            <label htmlFor="star3" title="text" />
            <input type="radio" id="star2" name="rate" defaultValue={2} />
            <label htmlFor="star2" title="text" />
            <input type="radio" id="star1" name="rate" defaultValue={1} />
            <label htmlFor="star1" title="text" />
          </div>
        </div>
        <div className="mb_32">
          <div className="mb_8">{t("reviews.reviewTitle")}</div>
          <fieldset className="mb_20">
            <input
              className=""
              type="text"
              placeholder={t("reviews.giveTitlePlaceholder")}
              name="text"
              tabIndex={2}
              defaultValue=""
              aria-required="true"
              required
            />
          </fieldset>
          <div className="mb_8">{t("reviews.review")}</div>
          <fieldset className="d-flex mb_20">
            <textarea
              className=""
              rows={4}
              placeholder={t("reviews.writePlaceholder")}
              tabIndex={2}
              aria-required="true"
              required
              defaultValue={""}
            />
          </fieldset>
          <div className="cols mb_20">
            <fieldset className="">
              <input
                className=""
                type="text"
                placeholder={t("reviews.yourNamePublic")}
                name="text"
                tabIndex={2}
                defaultValue=""
                aria-required="true"
                required
              />
            </fieldset>
            <fieldset className="">
              <input
                className=""
                type="email"
                placeholder={t("reviews.yourEmailPrivate")}
                name="email"
                tabIndex={2}
                defaultValue=""
                aria-required="true"
                required
              />
            </fieldset>
          </div>
          <div className="d-flex align-items-center check-save">
            <input
              type="checkbox"
              name="availability"
              className="tf-check"
              id="check1"
            />
            <label className="text-secondary text-caption-1" htmlFor="check1">
              {t("reviews.saveInfo")}
            </label>
          </div>
        </div>
        <div className="button-submit">
          <button className="text-btn-uppercase" type="submit">
            {t("reviews.submitReviews")}
          </button>
        </div>
      </form>
    </>
  );
}
