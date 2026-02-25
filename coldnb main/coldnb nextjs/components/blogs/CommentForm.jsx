"use client";
import { useLanguage } from "@/lib/i18n/LanguageContext";

export default function CommentForm() {
  const { t } = useLanguage();
  return (
    <div className="leave-comment">
      <h4 className="leave-comment-heading">{t("blog.leaveComment")}</h4>
      <form className="form-leave-comment" onSubmit={(e) => e.preventDefault()}>
        <div className="wrap">
          <div className="cols">
            <fieldset className="">
              <input
                className=""
                type="text"
                placeholder={t("contact.yourName")}
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
                placeholder={t("contact.yourEmail")}
                name="email"
                tabIndex={2}
                defaultValue=""
                aria-required="true"
                required
              />
            </fieldset>
          </div>
          <fieldset className="">
            <textarea
              className=""
              rows={4}
              placeholder={t("contact.yourMessage")}
              tabIndex={2}
              aria-required="true"
              required
              defaultValue={""}
            />
          </fieldset>
        </div>
        <div className="button-submit">
          <button className="" type="submit">
            {t("blog.submitReview")}
          </button>
        </div>
      </form>
    </div>
  );
}
